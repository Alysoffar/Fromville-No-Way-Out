#include "game/interactions/InteractionSystem.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>

#include "game/entities/Character.h"
#include "game/quest/QuestSystem.h"
#include "game/dialogue/DialogueManager.h"

namespace {
float HorizontalDistance(const glm::vec3& a, const glm::vec3& b) {
    glm::vec3 delta = a - b;
    delta.y = 0.0f;
    return glm::length(delta);
}

glm::vec3 GetCharacterForward(const Character& character) {
    const float yawRadians = glm::radians(character.transform.rotation.y);
    return glm::normalize(glm::vec3(std::sin(yawRadians), 0.0f, std::cos(yawRadians)));
}

bool IsShadowAligned(const Character& character, const InteractionNode& node) {
    glm::vec3 toTarget = node.position - character.transform.position;
    toTarget.y = 0.0f;
    if (glm::length(toTarget) <= 0.0001f) {
        return true;
    }

    toTarget = glm::normalize(toTarget);
    return glm::dot(GetCharacterForward(character), toTarget) > 0.98f;
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
    if (fields.size() >= 14) {
        node.questSubObjectiveIndex = std::stoi(fields[13]);
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

std::string InteractionSystem::GetPromptFor(const Character& character, const QuestSystem& questSystem, bool hasActiveQuest, CharacterType activeQuestCharacter) const {
    const InteractionNode* node = FindBestNode(character, questSystem);
    if (!node) {
        return "";
    }

    if (node->id == "boyd_evidence_idol" && !IsShadowAligned(character, *node)) {
        return "Hold the light on the idol";
    }

    if (node->questObjectiveIndex >= 0) {
        const Quest* quest = questSystem.GetCharacterQuest(character.GetType());
        const bool questComplete = quest && quest->IsComplete();
        const int nextObjectiveIndex = quest ? quest->GetNextIncompleteObjectiveIndex() : -1;

        if (quest && !questSystem.IsProgressAllowed(character.GetType(), questSystem.GetCurrentDayCyclePhase())) {
            const std::string lockMessage = questSystem.GetProgressLockMessage(character.GetType(), questSystem.GetCurrentDayCyclePhase());
            if (!lockMessage.empty()) {
                return lockMessage;
            }
            return "Wait for your progression window.";
        }

        if (!questComplete && hasActiveQuest && activeQuestCharacter != character.GetType()) {
            return "Finish your current quest first.";
        }

        if (!questComplete && nextObjectiveIndex != node->questObjectiveIndex) {
            return "Keep going with your current quest step.";
        }

        if (node->type == InteractionType::ItemPickup) {
            return "Collect " + node->name;
        } else {
            return node->prompt + " " + node->name;
        }
    }

    return node->prompt + " " + node->name;
}

bool InteractionSystem::TryInteract(Character& character, QuestSystem& questSystem, bool hasActiveQuest, CharacterType activeQuestCharacter) {
    lastInteractionMessage.clear();
    lastInteractionQuestObjectiveIndex = -1;
    lastInteractionQuestSubObjectiveIndex = -1;

    InteractionNode* node = FindBestNode(character, questSystem);
    if (!node || !node->active) {
        lastInteractionMessage = "Nothing nearby to interact with.";
        return false;
    }

    if (node->id == "boyd_evidence_idol" && !IsShadowAligned(character, *node)) {
        lastInteractionMessage = "The idol stays hidden until the light lines up.";
        return false;
    }

    if (node->questObjectiveIndex >= 0) {
        Quest* quest = questSystem.GetCharacterQuest(character.GetType());
        if (!quest) {
            lastInteractionMessage = "This quest cannot be started right now.";
            return false;
        }

        if (!questSystem.IsProgressAllowed(character.GetType(), questSystem.GetCurrentDayCyclePhase())) {
            lastInteractionMessage = questSystem.GetProgressLockMessage(character.GetType(), questSystem.GetCurrentDayCyclePhase());
            if (lastInteractionMessage.empty()) {
                lastInteractionMessage = "That progression window is closed right now.";
            }
            return false;
        }

        if (quest->IsComplete()) {
            lastInteractionMessage = "That quest is already complete.";
            return false;
        }

        if (hasActiveQuest && activeQuestCharacter != character.GetType()) {
            lastInteractionMessage = "Finish your current quest before starting another one.";
            return false;
        }

        if (quest->GetNextIncompleteObjectiveIndex() != node->questObjectiveIndex) {
            lastInteractionMessage = "You need to do the current quest step first.";
            return false;
        }
    }

    if (!node->requiredFlag.empty() && !questSystem.HasStoryFlag(node->requiredFlag)) {
        lastInteractionMessage = node->name + " is still locked off.";
        return false;
    }

    switch (node->type) {
        case InteractionType::Conversation:
        case InteractionType::Trigger:
        case InteractionType::ItemPickup:
            if (node->questObjectiveIndex >= 0) {
                lastInteractionQuestCharacter = character.GetType();
                lastInteractionQuestObjectiveIndex = node->questObjectiveIndex;
                lastInteractionQuestSubObjectiveIndex = node->questSubObjectiveIndex;
                lastInteractionMessage = "Puzzle opened: " + node->name;
            }

            lastInteractionMessage = node->successMessage;
            if (node->type == InteractionType::Conversation) {
                if (!DialogueManager::Instance().StartConversation(node->id)) {
                    DialogueManager::Instance().StartConversation(node->name);
                }
            }

            questSystem.SetStoryFlag(node->questFlag);
            if (node->questObjectiveIndex >= 0) {
                lastInteractionMessage = "Collected " + node->name + ". " + node->successMessage;
                bool isCollectObjective = false;
                Quest* quest = questSystem.GetCharacterQuest(character.GetType());
                if (quest) {
                    const auto& objectives = quest->GetObjectives();
                    if (node->questObjectiveIndex >= 0 && node->questObjectiveIndex < static_cast<int>(objectives.size())) {
                        isCollectObjective = (objectives[node->questObjectiveIndex].type == ObjectiveType::Collect);
                    }
                }

                if (isCollectObjective && (node->type == InteractionType::Conversation || node->type == InteractionType::Trigger)) {
                    node->collected = true;
                    node->active = false;
                }
            }
            break;
        case InteractionType::Door:
            node->open = !node->open;
            lastInteractionMessage = node->name + (node->open ? " opens. " : " closes. ") + node->successMessage;
            questSystem.SetStoryFlag(node->questFlag);
            break;
    }

    return true;
}

void InteractionSystem::MarkQuestStepSolved(CharacterType characterType, int objectiveIndex) {
    for (auto& node : nodes) {
        if (node.questObjectiveIndex == objectiveIndex && node.requiredCharacter == CharacterTypeName(characterType)) {
            node.collected = true;
            node.active = false;
        }
    }
}

void InteractionSystem::Update(float dt, QuestSystem& questSystem) {
    (void)dt;
    (void)questSystem;
}

bool InteractionSystem::LoadFromConfig(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return false;

    std::string line;
    while (std::getline(file, line)) {
        line = Trim(line);
        if (line.empty() || line.front() == '#') continue;
        const std::vector<std::string> fields = Split(line, '|');
        if (fields.size() < 12 || fields.size() > 14) continue;
        nodes.push_back(MakeNodeFromFields(fields));
    }
    return !nodes.empty();
}

void InteractionSystem::AddNode(const InteractionNode& node) {
    nodes.push_back(node);
}

bool InteractionSystem::TryPickup(Character& character, QuestSystem& questSystem, bool hasActiveQuest, CharacterType activeQuestCharacter) {
    lastInteractionMessage.clear();
    lastInteractionQuestObjectiveIndex = -1;
    lastInteractionQuestSubObjectiveIndex = -1;

    InteractionNode* node = FindBestNode(character, questSystem);
    if (!node || !node->active || node->type != InteractionType::ItemPickup) return false;

    if (node->id == "boyd_evidence_idol" && !IsShadowAligned(character, *node)) return false;

    if (node->questObjectiveIndex >= 0) {
        Quest* quest = questSystem.GetCharacterQuest(character.GetType());
        if (!quest || quest->IsComplete()) return false;
        if (hasActiveQuest && activeQuestCharacter != character.GetType()) return false;
        if (quest->GetNextIncompleteObjectiveIndex() != node->questObjectiveIndex) return false;
    }

    if (!node->requiredFlag.empty() && !questSystem.HasStoryFlag(node->requiredFlag)) return false;

    lastInteractionMessage = node->successMessage;
    questSystem.SetStoryFlag(node->questFlag);
    if (node->questObjectiveIndex >= 0) {
        lastInteractionMessage = "Collected " + node->name + ". " + node->successMessage;
        lastInteractionQuestCharacter = character.GetType();
        lastInteractionQuestObjectiveIndex = node->questObjectiveIndex;
        lastInteractionQuestSubObjectiveIndex = node->questSubObjectiveIndex;
    }

    node->collected = true;
    node->active = false;
    return true;
}

void InteractionSystem::LoadFallbackNodes() {
    nodes = {
        {"mara_conversation", InteractionType::Conversation, "Mara", glm::vec3(-4.0f, 0.0f, 2.5f), 2.5f, true, false, false, "Talk", "Mara remembers the town meetings and points Boyd toward the cult.", "mara_talked", "", 0, 0, "Boyd"},
        {"library_key", InteractionType::ItemPickup, "Library Key", glm::vec3(1.5f, 0.0f, 7.0f), 2.0f, true, false, false, "Pick up", "Jade picks up a key with a glyph etched into it.", "library_key_taken", "", 1, 0, "Jade"},
        {"tunnel_trigger", InteractionType::Trigger, "Collapsed Tunnel", glm::vec3(-8.0f, 0.0f, -6.0f), 3.0f, true, false, false, "Inspect", "Tabitha notices a hidden passage and marks it on the map.", "tunnel_found", "", 2, 0, "Tabitha"},
        {"victor_memory_trigger", InteractionType::Trigger, "Memory Site", glm::vec3(-2.0f, 0.0f, -10.0f), 3.0f, true, false, false, "Remember", "Victor gets hit by a memory fragment from the town's past.", "victor_memory_fragment", "tunnel_found", 3, 0, "Victor"}
    };
}

const InteractionNode* InteractionSystem::FindBestNode(const Character& character, const QuestSystem& questSystem) const {
    const InteractionNode* bestNode = nullptr;
    float bestDistance = 9999.0f;

    for (const auto& node : nodes) {
        if (!node.active && node.type != InteractionType::Door) continue;
        if (!node.requiredFlag.empty() && !questSystem.HasStoryFlag(node.requiredFlag)) continue;
        if (!node.requiredCharacter.empty() && node.requiredCharacter != CharacterTypeName(character.GetType())) continue;

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
    if (!node) return nullptr;
    for (auto& candidate : nodes) {
        if (candidate.id == node->id) return &candidate;
    }
    return nullptr;
}
