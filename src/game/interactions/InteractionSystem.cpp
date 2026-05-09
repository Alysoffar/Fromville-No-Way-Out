#include "game/interactions/InteractionSystem.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>

#include "game/entities/Character.h"
#include "game/quest/QuestSystem.h"

namespace {
float HorizontalDistance(const glm::vec3& a, const glm::vec3& b) {
    glm::vec3 delta = a - b;
    delta.y = 0.0f;
    return glm::length(delta);
}

std::string Trim(std::string value) {
    const auto notSpace = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
    value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());
    return value;
}

std::vector<std::string> Split(const std::string& line, char delimiter) {
    std::vector<std::string> parts;
    std::stringstream stream(line);
    std::string part;
    while (std::getline(stream, part, delimiter)) {
        parts.push_back(Trim(part));
    }
    return parts;
}

InteractionType ParseType(const std::string& value) {
    if (value == "Conversation") return InteractionType::Conversation;
    if (value == "ItemPickup") return InteractionType::ItemPickup;
    if (value == "Door") return InteractionType::Door;
    return InteractionType::Trigger;
}

std::string CharacterTypeName(CharacterType type) {
    switch (type) {
        case CharacterType::Boyd: return "Boyd";
        case CharacterType::Jade: return "Jade";
        case CharacterType::Tabitha: return "Tabitha";
        case CharacterType::Victor: return "Victor";
        case CharacterType::Sara: return "Sara";
    }

    return "";
}

InteractionNode MakeNodeFromFields(const std::vector<std::string>& fields) {
    InteractionNode node;
    node.id = fields[0];
    node.type = ParseType(fields[1]);
    node.name = fields[2];
    node.position = glm::vec3(std::stof(fields[3]), std::stof(fields[4]), std::stof(fields[5]));
    node.radius = std::stof(fields[6]);
    node.prompt = fields[7];
    node.successMessage = fields[8];
    node.questFlag = fields[9];
    node.requiredFlag = fields[10];
    node.questObjectiveIndex = std::stoi(fields[11]);
    if (fields.size() >= 13) {
        node.requiredCharacter = fields[12];
    }
    return node;
}
}

void InteractionSystem::Initialize() {
    nodes.clear();

    if (!LoadFromConfig("assets/config/interaction_nodes.txt")) {
        LoadFallbackNodes();
    }
}

std::string InteractionSystem::GetPromptFor(const Character& character, const QuestSystem& questSystem) const {
    const InteractionNode* node = FindBestNode(character, questSystem);
    if (!node) {
        return "";
    }

    return node->prompt + " " + node->name;
}

bool InteractionSystem::TryInteract(Character& character, QuestSystem& questSystem) {
    lastInteractionMessage.clear();

    InteractionNode* node = FindBestNode(character, questSystem);
    if (!node || !node->active) {
        lastInteractionMessage = "Nothing nearby to interact with.";
        return false;
    }

    if (!node->requiredFlag.empty() && !questSystem.HasStoryFlag(node->requiredFlag)) {
        lastInteractionMessage = node->name + " is still locked off.";
        std::cout << "[Interaction] " << lastInteractionMessage << "\n";
        return false;
    }

    switch (node->type) {
        case InteractionType::Conversation:
            lastInteractionMessage = node->successMessage;
            std::cout << "[Conversation] " << node->successMessage << "\n";
            if (node->questObjectiveIndex >= 0) {
                questSystem.AdvanceObjective(character.GetType(), node->questObjectiveIndex);
            }
            questSystem.SetStoryFlag(node->questFlag);
            break;
        case InteractionType::ItemPickup:
            if (!node->collected) {
                node->collected = true;
                node->active = false;
                lastInteractionMessage = node->successMessage;
                std::cout << "[Pickup] " << node->successMessage << "\n";
                questSystem.SetStoryFlag(node->questFlag);
                if (node->questObjectiveIndex >= 0) {
                    questSystem.AdvanceObjective(character.GetType(), node->questObjectiveIndex);
                }
            }
            break;
        case InteractionType::Door:
            node->open = !node->open;
            lastInteractionMessage = node->name + (node->open ? " opens. " : " closes. ") + node->successMessage;
            std::cout << "[Door] " << lastInteractionMessage << "\n";
            questSystem.SetStoryFlag(node->questFlag);
            break;
        case InteractionType::Trigger:
            lastInteractionMessage = node->successMessage;
            std::cout << "[Trigger] " << node->successMessage << "\n";
            questSystem.SetStoryFlag(node->questFlag);
            if (node->questObjectiveIndex >= 0) {
                questSystem.AdvanceObjective(character.GetType(), node->questObjectiveIndex);
            }
            break;
    }

    return true;
}

void InteractionSystem::Update(float dt, QuestSystem& questSystem) {
    (void)dt;
    (void)questSystem;
}

bool InteractionSystem::LoadFromConfig(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[InteractionSystem] Could not open " << path << ", using built-in defaults.\n";
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        line = Trim(line);
        if (line.empty() || line.front() == '#') {
            continue;
        }

        const std::vector<std::string> fields = Split(line, '|');
        if (fields.size() != 12 && fields.size() != 13) {
            std::cerr << "[InteractionSystem] Skipping malformed node line: " << line << "\n";
            continue;
        }

        nodes.push_back(MakeNodeFromFields(fields));
    }

    std::cout << "[InteractionSystem] Loaded " << nodes.size() << " interaction nodes from config.\n";
    return !nodes.empty();
}

void InteractionSystem::LoadFallbackNodes() {
    nodes = {
        {"mara_conversation", InteractionType::Conversation, "Mara", glm::vec3(-4.0f, 0.0f, 2.5f), 2.5f, true, false, false, "Talk", "Mara remembers the town meetings and points Boyd toward the cult.", "mara_talked", "", 0, "Boyd"},
        {"library_key", InteractionType::ItemPickup, "Library Key", glm::vec3(1.5f, 0.0f, 7.0f), 2.0f, true, false, false, "Pick up", "Jade picks up a key with a glyph etched into it.", "library_key_taken", "", 1, "Jade"},
        {"cabin_door", InteractionType::Door, "Cabin Door", glm::vec3(7.0f, 0.0f, 3.0f), 2.2f, true, false, false, "Open/Close", "The cabin door unlocks and reveals a safe space.", "cabin_door_opened", "library_key_taken", -1},
        {"tunnel_trigger", InteractionType::Trigger, "Collapsed Tunnel", glm::vec3(-8.0f, 0.0f, -6.0f), 3.0f, true, false, false, "Inspect", "Tabitha notices a hidden passage and marks it on the map.", "tunnel_found", "", 2, "Tabitha"},
        {"victor_memory_trigger", InteractionType::Trigger, "Memory Site", glm::vec3(-2.0f, 0.0f, -10.0f), 3.0f, true, false, false, "Remember", "Victor gets hit by a memory fragment from the town's past.", "victor_memory_fragment", "tunnel_found", 3, "Victor"},
        {"sara_whisper_trigger", InteractionType::Trigger, "Whispering Point", glm::vec3(10.0f, 0.0f, -3.0f), 3.0f, true, false, false, "Listen", "Sara hears a warning and learns who is being watched.", "sara_whisper_heard", "", 4, "Sara"},
        {"sheriff_talisman", InteractionType::Trigger, "Sheriff Station Talisman", glm::vec3(0.0f, 0.0f, 8.0f), 3.0f, true, false, false, "Check", "The talisman holds. The monsters circle outside but cannot enter.", "sheriff_talisman_checked", "", -1},
        {"diner_shelter", InteractionType::Trigger, "Diner Shelter", glm::vec3(-6.0f, 0.0f, 1.0f), 3.0f, true, false, false, "Secure", "The doors are sealed and the townspeople settle in for the night.", "diner_secured", "", -1},
        {"colony_house_warning", InteractionType::Trigger, "Colony House Window", glm::vec3(9.0f, 0.0f, 8.0f), 3.0f, true, false, false, "Inspect", "A window latch is loose. At night this would break the protection.", "colony_window_checked", "", -1},
        {"church_symbol", InteractionType::Trigger, "Church Symbol", glm::vec3(8.0f, 0.0f, -2.0f), 2.5f, true, false, false, "Decode", "Jade recognizes the pattern and links it to the tunnels.", "church_symbol_decoded", "", 1, "Jade"},
        {"tunnel_entrance", InteractionType::Door, "Tunnel Entrance", glm::vec3(-8.0f, 0.0f, -6.0f), 3.0f, true, false, false, "Open/Close", "The tunnel entrance opens into a placeholder underground route.", "tunnel_entrance_opened", "tunnel_found", -1},
        {"victor_lunchbox", InteractionType::ItemPickup, "Victor's Lunchbox", glm::vec3(-9.0f, 0.0f, 5.0f), 2.0f, true, false, false, "Pick up", "Victor recovers a drawing that marks a safe path through town.", "victor_lunchbox_found", "", 3, "Victor"}
    };
}

const InteractionNode* InteractionSystem::FindBestNode(const Character& character, const QuestSystem& questSystem) const {
    const InteractionNode* bestNode = nullptr;
    float bestDistance = 9999.0f;

    for (const auto& node : nodes) {
        if (!node.active && node.type != InteractionType::Door) {
            continue;
        }

        if (!node.requiredFlag.empty() && !questSystem.HasStoryFlag(node.requiredFlag)) {
            continue;
        }

        if (!node.requiredCharacter.empty() && node.requiredCharacter != CharacterTypeName(character.GetType())) {
            continue;
        }

        const float distance = HorizontalDistance(character.transform.position, node.position);
        if (distance <= node.radius && distance < bestDistance) {
            bestDistance = distance;
            bestNode = &node;
        }
    }

    return bestNode;
}

InteractionNode* InteractionSystem::FindBestNode(const Character& character, const QuestSystem& questSystem) {
    const InteractionSystem* constThis = this;
    const InteractionNode* node = constThis->FindBestNode(character, questSystem);
    if (!node) {
        return nullptr;
    }

    for (auto& candidate : nodes) {
        if (candidate.id == node->id) {
            return &candidate;
        }
    }

    return nullptr;
}
