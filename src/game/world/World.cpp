#include "game/world/World.h"

#include <cfloat>
#include <cmath>
#include <cctype>
#include <iomanip>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>

#include "engine/renderer/Camera.h"
#include "engine/audio/AudioManager.h"
#include "engine/renderer/Mesh.h"
#include "engine/renderer/Shader.h"
#include "engine/renderer/TextRenderer.h"
#include "engine/core/InputManager.h"
#include "engine/resources/loader.h"
#include "game/atmosphere/AtmosphereManager.h"
#include "game/memory/MemoryReplaySystem.h"
#include "game/moral/MoralCorruptionSystem.h"
#include "game/symbols/SymbolSystem.h"
#include "game/tunnels/TunnelMappingSystem.h"
#include "game/runtime/GameplayEvents.h"
#include "game/world/map_manager.h"
#include "game/story/StoryManager.h"
#include "game/world/DayNightCycle.h"

namespace {

// ====== Terrain project: building mesh system ======
struct WorldMesh {
    Mesh mesh;
    float minY = 0.0f;
    bool valid = false;
};

WorldMesh gHouseMesh;
WorldMesh gDinnerMesh;
WorldMesh gPoliceMesh;
Shader gBuildingShader("BuildingModel");
bool gBuildingReady = false;

float CalculateMinY(const std::vector<MeshVertex>& vertices) {
    float minY = FLT_MAX;
    for (const auto& v : vertices) {
        minY = std::min(minY, v.position.y);
    }
    return minY;
}

void LoadBuildingMeshes(CollisionWorld* cw, std::vector<Door>& doors) {
    if (gBuildingReady) return;

    std::cout << "[World] Loading building meshes...\n";

    auto LoadBuilding = [&](const std::string& path, WorldMesh& outMesh, glm::vec3 pos, float rot, float scale, const std::string& doorKeyword, float doorOpenAngle) {
        std::vector<OBJShape> shapes;
        if (!Loader::LoadOBJWithShapes(path, shapes)) return;

        std::vector<MeshVertex> mainVertices;
        std::vector<unsigned int> mainIndices;

        float globalMinY = FLT_MAX;
        glm::vec3 localMin(FLT_MAX);
        glm::vec3 localMax(-FLT_MAX);
        for (const auto& s : shapes) {
            for (const auto& v : s.vertices) {
                globalMinY = std::min(globalMinY, v.position.y);
                localMin = glm::min(localMin, v.position * scale);
                localMax = glm::max(localMax, v.position * scale);
            }
        }
        float adjustedY = -globalMinY * scale + 0.05f;
        glm::vec3 buildingPos = glm::vec3(pos.x, adjustedY, pos.z);

        glm::vec3 localCenter = (localMin + localMax) * 0.5f;
        glm::mat4 localToWorld = glm::translate(glm::mat4(1.0f), buildingPos);
        localToWorld = glm::rotate(localToWorld, glm::radians(rot), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::vec3 buildingWorldCenter = glm::vec3(localToWorld * glm::vec4(localCenter, 1.0f));
        buildingWorldCenter.y = buildingPos.y + 0.5f;

        std::map<std::string, std::pair<std::vector<MeshVertex>, std::vector<unsigned int>>> doorGroups;

        for (auto& s : shapes) {
            if (s.name.find(doorKeyword) != std::string::npos &&
                s.name.find("Frame") == std::string::npos &&
                s.name.find("Sill") == std::string::npos) {

                std::string groupName = s.name;
                if (s.name.find("Knob") != std::string::npos) {
                    groupName = "Door_Main_Mesh";
                }

                auto& group = doorGroups[groupName];
                unsigned int offset = static_cast<unsigned int>(group.first.size());
                for (auto v : s.vertices) {
                    if (scale != 1.0f) v.position *= scale;
                    group.first.push_back(v);
                }
                for (auto i : s.indices) group.second.push_back(i + offset);
            } else {
                unsigned int offset = static_cast<unsigned int>(mainVertices.size());
                for (auto v : s.vertices) {
                    if (scale != 1.0f) v.position *= scale;
                    mainVertices.push_back(v);
                }
                for (auto i : s.indices) mainIndices.push_back(i + offset);
            }
        }

        for (auto& [dName, dData] : doorGroups) {
            if (dData.first.empty()) continue;

            glm::vec3 minP(FLT_MAX), maxP(-FLT_MAX);
            for (const auto& v : dData.first) {
                minP = glm::min(minP, v.position);
                maxP = glm::max(maxP, v.position);
            }

            float hingeX = (dName.find("_R") != std::string::npos) ? maxP.x : minP.x;
            glm::vec3 hingePos = glm::vec3(hingeX, 0.0f, minP.z);

            float angle = doorOpenAngle;
            if (dName.find("_R") != std::string::npos) {
                angle = -doorOpenAngle;
            }

            glm::mat4 doorLocalToWorld = glm::translate(glm::mat4(1.0f), buildingPos);
            doorLocalToWorld = glm::rotate(doorLocalToWorld, glm::radians(rot), glm::vec3(0.0f, 1.0f, 0.0f));
            glm::vec3 doorWorldCenter = glm::vec3(doorLocalToWorld * glm::vec4(hingePos, 1.0f));

            Door d;
            d.Initialize(dName, dData.first, dData.second, buildingPos, rot, angle, hingePos, doorWorldCenter);
            d.SetInsidePosition(buildingWorldCenter);
            doors.push_back(std::move(d));
        }

        if (doorGroups.empty()) {
            glm::vec3 entranceLocalPos(0.0f);
            int stepVertCount = 0;
            for (const auto& s : shapes) {
                if (s.name.find("Step") != std::string::npos) {
                    for (const auto& v : s.vertices) {
                        entranceLocalPos += v.position * scale;
                        stepVertCount++;
                    }
                }
            }

            if (stepVertCount > 0) {
                entranceLocalPos /= static_cast<float>(stepVertCount);
            }

            glm::mat4 entranceLocalToWorld = glm::translate(glm::mat4(1.0f), buildingPos);
            entranceLocalToWorld = glm::rotate(entranceLocalToWorld, glm::radians(rot), glm::vec3(0.0f, 1.0f, 0.0f));
            glm::vec3 entranceWorldPos = glm::vec3(entranceLocalToWorld * glm::vec4(entranceLocalPos, 1.0f));

            Door virtualDoor;
            virtualDoor.Initialize(path + "_Entrance", {}, {}, buildingPos, rot, 0.0f, glm::vec3(0.0f), entranceWorldPos);
            glm::vec3 insidePos = buildingPos;
            insidePos.y += 0.5f;
            virtualDoor.SetInsidePosition(insidePos);
            doors.push_back(std::move(virtualDoor));
        }

        outMesh.mesh.Create(mainVertices, mainIndices);
        outMesh.minY = CalculateMinY(mainVertices);
        outMesh.valid = outMesh.mesh.IsValid();

        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(pos.x, adjustedY, pos.z));
        model = glm::rotate(model, glm::radians(rot), glm::vec3(0.0f, 1.0f, 0.0f));
        if (cw) cw->AddTrianglesFromMesh(mainVertices, mainIndices, model, true);
    };

    LoadBuilding("assets/models/House.obj", gHouseMesh, glm::vec3(-25.0f, 0, 0.0f), 90.0f, 1.0f, "Door_Main", 90.0f);
    LoadBuilding("assets/models/House.obj", gHouseMesh, glm::vec3(25.0f, 0, 0.0f), -90.0f, 1.0f, "Door_Main", 90.0f);
    LoadBuilding("assets/models/House.obj", gHouseMesh, glm::vec3(-25.0f, 0, 20.0f), 90.0f, 1.0f, "Door_Main", 90.0f);
    LoadBuilding("assets/models/House.obj", gHouseMesh, glm::vec3(25.0f, 0, 20.0f), -90.0f, 1.0f, "Door_Main", 90.0f);
    LoadBuilding("assets/models/House.obj", gHouseMesh, glm::vec3(-25.0f, 0, -20.0f), 90.0f, 1.0f, "Door_Main", 90.0f);
    LoadBuilding("assets/models/House.obj", gHouseMesh, glm::vec3(25.0f, 0, -20.0f), -90.0f, 1.0f, "Door_Main", 90.0f);
    LoadBuilding("assets/models/House.obj", gHouseMesh, glm::vec3(0.0f, 0, 20.0f), 0.0f, 1.0f, "Door_Main", 90.0f);

    // Align the main landmarks with the story-location coordinates used by quests and shelters.
    LoadBuilding("assets/models/Dinner.obj", gDinnerMesh, glm::vec3(-35.0f, 0, -35.0f), 0.0f, 1.0f, "DoorGlass", -90.0f);
    LoadBuilding("assets/models/House.obj", gHouseMesh, glm::vec3(9.0f, 0, 8.0f), 0.0f, 1.0f, "Door_Main", 90.0f);
    LoadBuilding("assets/models/Police.obj", gPoliceMesh, glm::vec3(-9.0f, 0, 8.0f), 180.0f, 2.0f, "Door", 90.0f);

    gBuildingShader.Load("assets/shaders/model_lit.vert", "assets/shaders/model_lit.frag");
    gBuildingReady = true;
}
// ====== End terrain building system ======

Mesh gPlayerMesh;
Shader gPlayerShader("PlayerModel");
bool gPlayerReady = false;

Mesh gNpcMesh;
Mesh gEnemyMesh;
Mesh gQuestMarkerMesh;
Shader gCharacterShader("CharacterCubes");
bool gCharacterReady = false;
bool gQuestMarkerReady = false;

constexpr float kNpcSightRange = 9.0f;
constexpr float kEnemySightRange = 18.0f;
constexpr float kEnemyHearingRange = 26.0f;
constexpr float kEnemyLightRange = 22.0f;
constexpr float kArenaHalfExtent = 128.0f;

struct VoiceCueLibrary {
    bool initialized = false;
    std::unordered_map<std::string, std::string> monster;
    std::unordered_map<std::string, std::string> npcFemale;
    std::unordered_map<std::string, std::string> npcMale;
    std::unordered_map<std::string, std::string> responses;
    std::unordered_map<std::string, std::string> panicFemale;
    std::unordered_map<std::string, std::string> panicMale;
};

VoiceCueLibrary& GetVoiceCueLibrary() {
    static VoiceCueLibrary library;
    return library;
}

std::string BuildVoiceSlug(const std::string& line) {
    std::string cleaned;
    cleaned.reserve(line.size());

    bool prevUnderscore = false;
    for (char c : line) {
        const unsigned char uc = static_cast<unsigned char>(c);
        if (std::isalnum(uc)) {
            cleaned.push_back(static_cast<char>(std::tolower(uc)));
            prevUnderscore = false;
        } else if (!prevUnderscore) {
            cleaned.push_back('_');
            prevUnderscore = true;
        }
    }

    while (!cleaned.empty() && cleaned.front() == '_') {
        cleaned.erase(cleaned.begin());
    }
    while (!cleaned.empty() && cleaned.back() == '_') {
        cleaned.pop_back();
    }

    if (cleaned.size() > 40) {
        cleaned = cleaned.substr(0, 40);
        while (!cleaned.empty() && cleaned.back() == '_') {
            cleaned.pop_back();
        }
    }

    if (cleaned.empty()) {
        cleaned = "line";
    }
    return cleaned;
}

std::string NormalizeVoiceLine(std::string line) {
    const std::string monsterPrefix = "[MONSTER]";
    if (line.rfind(monsterPrefix, 0) == 0) {
        line = line.substr(monsterPrefix.size());
        while (!line.empty() && std::isspace(static_cast<unsigned char>(line.front()))) {
            line.erase(line.begin());
        }
    }
    return line;
}

std::string StripSpeakerPrefix(const std::string& line) {
    const std::size_t colon = line.find(':');
    if (colon == std::string::npos || colon + 1 >= line.size()) {
        return line;
    }

    std::string stripped = line.substr(colon + 1);
    while (!stripped.empty() && std::isspace(static_cast<unsigned char>(stripped.front()))) {
        stripped.erase(stripped.begin());
    }
    return stripped;
}

bool IsFemaleSpeaker(const std::string& name) {
    return name == "Elena" || name == "Mara" || name == "Tabitha" || name == "Sara";
}

std::string GetSpeakerName(const std::string& line) {
    const std::size_t toPos = line.find(" to ");
    if (toPos != std::string::npos) {
        return line.substr(0, toPos);
    }

    const std::size_t colonPos = line.find(':');
    if (colonPos != std::string::npos) {
        return line.substr(0, colonPos);
    }

    return "";
}

void RegisterVoiceFolder(
    AudioManager& audio,
    const std::filesystem::path& folder,
    const std::string& cuePrefix,
    std::unordered_map<std::string, std::string>& outMap) {
    if (!std::filesystem::exists(folder)) {
        return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(folder)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".wav") {
            continue;
        }

        const std::string stem = entry.path().stem().string();
        const std::size_t underscore = stem.find('_');
        if (underscore == std::string::npos || underscore + 1 >= stem.size()) {
            continue;
        }

        const std::string slug = stem.substr(underscore + 1);
        const std::string cueName = cuePrefix + "/" + entry.path().filename().string();
        if (audio.LoadSound(cueName, entry.path().string())) {
            outMap[slug] = cueName;
        }
    }
}

void InitializeVoiceCueLibrary(AudioManager& audio) {
    VoiceCueLibrary& library = GetVoiceCueLibrary();
    if (library.initialized) {
        return;
    }

    RegisterVoiceFolder(audio, "assets/audio/voice/monster", "voice/monster", library.monster);
    RegisterVoiceFolder(audio, "assets/audio/voice/npc_female", "voice/npc_female", library.npcFemale);
    RegisterVoiceFolder(audio, "assets/audio/voice/npc_male", "voice/npc_male", library.npcMale);
    RegisterVoiceFolder(audio, "assets/audio/voice/responses", "voice/responses", library.responses);
    RegisterVoiceFolder(audio, "assets/audio/voice/panic_female", "voice/panic_female", library.panicFemale);
    RegisterVoiceFolder(audio, "assets/audio/voice/panic_male", "voice/panic_male", library.panicMale);

    library.initialized = true;
}

bool PlayVoiceLine(AudioManager* audio, const std::string& group, const std::string& rawLine, float gain = 0.85f) {
    if (!audio || rawLine.empty()) {
        return false;
    }

    VoiceCueLibrary& library = GetVoiceCueLibrary();
    if (!library.initialized) {
        InitializeVoiceCueLibrary(*audio);
    }

    std::string line = NormalizeVoiceLine(rawLine);
    if (group == "npc_female" || group == "npc_male") {
        line = StripSpeakerPrefix(line);
    }

    const std::string slug = BuildVoiceSlug(line);
    const std::unordered_map<std::string, std::string>* table = nullptr;
    if (group == "monster") {
        table = &library.monster;
    } else if (group == "npc_female") {
        table = &library.npcFemale;
    } else if (group == "npc_male") {
        table = &library.npcMale;
    } else if (group == "responses") {
        table = &library.responses;
    } else if (group == "panic_female") {
        table = &library.panicFemale;
    } else if (group == "panic_male") {
        table = &library.panicMale;
    }

    if (!table) {
        return false;
    }

    const auto found = table->find(slug);
    if (found == table->end()) {
        return false;
    }

    return audio->PlaySound(found->second, gain);
}

float HorizontalDistance(const glm::vec3& a, const glm::vec3& b) {
    glm::vec3 delta = a - b;
    delta.y = 0.0f;
    return glm::length(delta);
}

std::string CharacterTypeToName(CharacterType type) {
    switch (type) {
        case CharacterType::Boyd: return "Boyd";
        case CharacterType::Jade: return "Jade";
        case CharacterType::Tabitha: return "Tabitha";
        case CharacterType::Victor: return "Victor";
        case CharacterType::Sara: return "Sara";
    }

    return "";
}

const InteractionNode* FindNearestInteractionNode(const std::vector<InteractionNode>& nodes, const Character& character, const QuestSystem& questSystem) {
    const InteractionNode* bestNode = nullptr;
    float bestDistance = std::numeric_limits<float>::max();

    for (const InteractionNode& node : nodes) {
        if (!node.active && node.type != InteractionType::Door) {
            continue;
        }

        if (!node.requiredFlag.empty() && !questSystem.HasStoryFlag(node.requiredFlag)) {
            continue;
        }

        if (!node.requiredCharacter.empty() && node.requiredCharacter != CharacterTypeToName(character.GetType())) {
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

const InteractionNode* FindQuestInteractionNode(
    const std::vector<InteractionNode>& nodes,
    const Character* playerChar,
    const QuestSystem& questSystem,
    CharacterType questCharacter,
    int objectiveIndex,
    const QuestObjective& currentObjective) {
    const std::string requiredCharacter = CharacterTypeToName(questCharacter);
    const InteractionNode* activeNode = nullptr;
    float bestDistance = std::numeric_limits<float>::max();

    for (const InteractionNode& node : nodes) {
        if (node.questObjectiveIndex != objectiveIndex || node.requiredCharacter != requiredCharacter) {
            continue;
        }
        if (!node.active && node.type != InteractionType::Door) {
            continue;
        }
        if (!node.requiredFlag.empty() && !questSystem.HasStoryFlag(node.requiredFlag)) {
            continue;
        }
        if (currentObjective.type == ObjectiveType::Collect && !node.questFlag.empty() && questSystem.HasStoryFlag(node.questFlag)) {
            continue;
        }

        const float distance = playerChar ? HorizontalDistance(playerChar->transform.position, node.position) : 0.0f;
        if (!activeNode || distance < bestDistance) {
            activeNode = &node;
            bestDistance = distance;
        }
    }

    return activeNode;
}

float Clamp01(float value) {
    return glm::clamp(value, 0.0f, 1.0f);
}

EnemyPerception EvaluateTargetStimulus(
    const glm::vec3& observer,
    const glm::vec3& target,
    float sightRange,
    float soundRange,
    float lightRange,
    float soundAmount,
    float lightAmount) {
    const float distance = HorizontalDistance(observer, target);
    EnemyPerception result;
    result.targetPosition = target;
    result.visible = distance <= sightRange;
    result.proximity = Clamp01(1.0f - (distance / sightRange));
    result.sound = soundAmount * Clamp01(1.0f - (distance / soundRange));
    result.light = lightAmount * Clamp01(1.0f - (distance / lightRange));
    result.hasTarget = result.visible || result.sound > 0.05f || result.light > 0.05f;
    return result;
}

float ScorePerception(const EnemyPerception& perception) {
    return perception.proximity * 0.45f + perception.sound * 0.35f + perception.light * 0.20f;
}

glm::vec3 ClampArenaPosition(const glm::vec3& position) {
    glm::vec3 clamped = position;
    clamped.x = glm::clamp(clamped.x, -kArenaHalfExtent, kArenaHalfExtent);
    clamped.z = glm::clamp(clamped.z, -kArenaHalfExtent, kArenaHalfExtent);
    clamped.y = glm::max(clamped.y, 0.0f);
    return clamped;
}

std::string GetNpcLine(const NPC& npc, const Character& character, float worldClock) {
    // Generate unique dialogue based on NPC, character, and time
    static const std::array<std::string, 30> generalLines = {
        "This place is too quiet.",
        "Stick close and keep moving.",
        "I heard a scream. Or maybe that was just the wind.",
        "If you find anything strange, tell the others.",
        "Stay alert. Something doesn't feel right here.",
        "Did you hear that sound?",
        "We should stick together.",
        "The air feels cold... wrong.",
        "I'm not sure which way is safe anymore.",
        "Keep your eyes open.",
        "This silence is worse than noise.",
        "I don't like this place.",
        "Maybe we should find shelter.",
        "Do you feel that? Like we're being watched?",
        "Whatever's out here... it's not natural.",
        "Let's not stay in one place too long.",
        "The lights flickered twice near the road.",
        "Someone left footprints by the trees again.",
        "I keep hearing my name from empty rooms.",
        "The radio only gives static after sunset.",
        "If the houses go dark, run for a talisman.",
        "There was a knock from inside a sealed room.",
        "Nobody goes near the edge of town alone.",
        "The monsters smile like they know us.",
        "I saw something watching from the second floor.",
        "The diner is safer if we move before dark.",
        "Somebody needs to check the windows.",
        "The tunnels are whispering again.",
        "The town feels like it moved overnight.",
        "If you hear music, do not follow it."
    };
    
    // Select based on NPC character hash and world time
    const std::size_t npcHash = std::hash<std::string>{}(npc.GetName());
    const std::size_t charHash = static_cast<std::size_t>(character.GetType());
    const std::size_t timeComponent = static_cast<std::size_t>(std::fmod(worldClock, 10.0f));
    const std::size_t index = (npcHash + charHash * 7 + timeComponent * 13) % generalLines.size();
    
    return generalLines[index];
}

std::string GetCharacterNpcReplyLine(const NPC& npc, const Character& character, float worldClock) {
    static const std::array<std::string, 6> boydLines = {
        "I will check it. Stay where I can see you.",
        "Nobody wanders off. Not tonight.",
        "Tell me exactly what you heard.",
        "Get inside if the sky starts to turn.",
        "We keep calm, then we move.",
        "I need names, places, anything useful."
    };
    static const std::array<std::string, 6> jadeLines = {
        "That sounds like a pattern, not a coincidence.",
        "Show me where. I need to compare the symbols.",
        "Great. The nightmare has rules and nobody wrote them down.",
        "If it repeats, we can use it.",
        "Static, whispers, lights. It all connects somehow.",
        "Do not touch anything strange until I see it."
    };
    static const std::array<std::string, 6> tabithaLines = {
        "I have seen those signs underground.",
        "If there is a passage, I can find it.",
        "Keep the children away from the doors.",
        "The house sounds different when something is below it.",
        "I know that feeling. The town is hiding spaces.",
        "Mark the place and do not go back alone."
    };
    static const std::array<std::string, 6> victorLines = {
        "That happened before. I remember the color of the sky.",
        "The trees know when people are scared.",
        "Do not follow the music. It lies.",
        "I can draw it if you let me think.",
        "Some places are hungry. You should leave them alone.",
        "The town keeps the old things close."
    };
    static const std::array<std::string, 6> saraLines = {
        "The voices get louder near places like that.",
        "I am trying to listen without obeying.",
        "If I hear a warning, I will tell you.",
        "Sometimes the town uses fear like a hook.",
        "Do not trust anything that says it can save us.",
        "I know what it feels like when it calls your name."
    };

    const std::size_t npcHash = std::hash<std::string>{}(npc.GetName());
    const std::size_t index = (npcHash + static_cast<std::size_t>(worldClock)) % boydLines.size();
    switch (character.GetType()) {
        case CharacterType::Boyd: return boydLines[index];
        case CharacterType::Jade: return jadeLines[index];
        case CharacterType::Tabitha: return tabithaLines[index];
        case CharacterType::Victor: return victorLines[index];
        case CharacterType::Sara: return saraLines[index];
    }

    return "Stay close.";
}

std::string GetCharacterPairLine(const Character& activeCharacter, const Character& otherCharacter, float worldClock) {
    static const std::array<std::string, 10> lines = {
        "Keep your voice down. Sound carries here.",
        "If we split up, we meet back at the lights.",
        "You saw that too, right?",
        "The town is trying to herd us somewhere.",
        "Check the windows before dark.",
        "We need to compare what everyone found.",
        "If something smiles at you, do not answer.",
        "Stay close until we know where the monsters are.",
        "There is always a rule. We just have to find it.",
        "This place changes when nobody is looking."
    };

    const std::size_t index = (static_cast<std::size_t>(activeCharacter.GetType()) * 5 +
                               static_cast<std::size_t>(otherCharacter.GetType()) * 3 +
                               static_cast<std::size_t>(worldClock)) % lines.size();
    return activeCharacter.GetName() + " to " + otherCharacter.GetName() + ": " + lines[index];
}

std::string GetNpcNeighborLine(const NPC& speaker, const NPC& listener, float worldClock) {
    static const std::array<std::string, 12> lines = {
        "Did you lock the back door?",
        "The lamps near the road went out again.",
        "I counted three of them before sunrise.",
        "Keep everyone away from the tree line.",
        "The diner still has supplies if we hurry.",
        "I heard knocking from the empty house.",
        "Nobody sleeps near a window tonight.",
        "If you see a child outside, call Boyd first.",
        "The church bell rang but nobody was there.",
        "Something copied my voice yesterday.",
        "We move together when it gets dark.",
        "Tom said the road signs changed again."
    };

    const std::size_t speakerHash = std::hash<std::string>{}(speaker.GetName());
    const std::size_t listenerHash = std::hash<std::string>{}(listener.GetName());
    const std::size_t index = (speakerHash + listenerHash * 3 + static_cast<std::size_t>(worldClock)) % lines.size();
    return speaker.GetName() + " to " + listener.GetName() + ": " + lines[index];
}

std::string GetPanicLine(const Character& character) {
    static const std::array<std::string, 12> lines = {
        "Monster!",
        "Run!",
        "Get away from me!",
        "No, no, no!",
        "Someone help!",
        "Do not look at its face!",
        "Inside, now!",
        "It knows my name!",
        "The talisman, get to the talisman!",
        "Keep moving!",
        "It is too close!",
        "Shut the door!"
    };

    return lines[static_cast<std::size_t>(character.GetType()) % lines.size()];
}

void SeedNarrativeSystems() {
    AtmosphereManager::Instance().SetGlobalTension(0.28f);
    AtmosphereManager::Instance().SetCorruption(0.06f);

    MoralCorruptionSystem& moralSystem = MoralCorruptionSystem::Instance();
    if (moralSystem.GetAllEvidence().empty()) {
        moralSystem.RegisterEvidence({"Witness Statement", "A witness places the cult near the chapel.", MoralChoice::Exposure});
        moralSystem.RegisterEvidence({"Smeared Key", "A key found near the tunnel entrance.", MoralChoice::Mercy});
        moralSystem.RegisterEvidence({"Blood Note", "A torn note linking the town leadership to the disappearances.", MoralChoice::Revenge});
    }
}

void TriggerQuestlineNarrativeHooks(CharacterType characterType, int objectiveIndex, const Quest* quest, const glm::vec3& focusPosition) {
    if (!quest || objectiveIndex < 0) {
        return;
    }

    const auto& objectives = quest->GetObjectives();
    if (objectiveIndex >= static_cast<int>(objectives.size())) {
        return;
    }

    const QuestObjective& objective = objectives[objectiveIndex];
    AtmosphereManager& atmosphere = AtmosphereManager::Instance();
    SymbolSystem& symbolSystem = SymbolSystem::Instance();
    TunnelMappingSystem& tunnelSystem = TunnelMappingSystem::Instance();
    MemoryReplaySystem& memorySystem = MemoryReplaySystem::Instance();
    MoralCorruptionSystem& moralSystem = MoralCorruptionSystem::Instance();

    atmosphere.IncreaseTension(0.04f);

    switch (characterType) {
        case CharacterType::Boyd:
            if (objective.type == ObjectiveType::Combat) {
                atmosphere.TriggerEvent(AtmosphereEvent::DistantSinging, focusPosition, 0.7f);
            } else if (objective.type == ObjectiveType::Environmental) {
                atmosphere.TriggerEvent(AtmosphereEvent::WallBleeding, focusPosition, 0.45f);
            }
            break;
        case CharacterType::Jade:
            symbolSystem.AdvanceDecoding(0.18f);
            atmosphere.TriggerEvent(AtmosphereEvent::SymbolRearrangement, focusPosition, 0.8f);
            if (objectiveIndex == 0) {
                symbolSystem.RevealSymbols(3.0f);
            } else if (objectiveIndex == 4) {
                symbolSystem.TriggerSymbolRearrangement();
            }
            break;
        case CharacterType::Tabitha:
            if (objectiveIndex == 0) {
                tunnelSystem.DiscoverNode("North Passage");
            } else if (objectiveIndex == 1) {
                tunnelSystem.DiscoverNode("East Chamber");
            } else if (objectiveIndex == 2) {
                tunnelSystem.FloatThroughWater(2.5f);
            } else if (objectiveIndex == 3) {
                tunnelSystem.DiscoverNode("West Shrine");
            } else if (objectiveIndex == 4) {
                tunnelSystem.ReachCentralChamber();
            }
            atmosphere.TriggerEvent(AtmosphereEvent::ShadowMovement, focusPosition, 0.55f);
            break;
        case CharacterType::Victor:
            memorySystem.ActivateMemoryMode(3.5f);
            memorySystem.ProcessTraumaticMemory(objective.description);
            memorySystem.TriggerGhostReplica("Victor", objective.description, focusPosition);
            atmosphere.TriggerEvent(AtmosphereEvent::MemoryFlash, focusPosition, 0.9f);
            break;
        case CharacterType::Sara:
            moralSystem.OverhearConversation("Town secrets", objective.description);
            moralSystem.DetectGuilt("Thomas Reed", 0.08f);
            if (objectiveIndex == 4) {
                moralSystem.RecordChoice(MoralChoice::Exposure);
            }
            atmosphere.TriggerEvent(AtmosphereEvent::CorruptionPulse, focusPosition, 0.9f);
            break;
    }
}

void TriggerDialogueNarrativeHooks(Character& character, const NPC& npc, const std::string& npcLine, const std::string& characterReply) {
    const glm::vec3 focusPosition = npc.transform.position;

    switch (character.GetType()) {
        case CharacterType::Boyd:
            AtmosphereManager::Instance().TriggerEvent(AtmosphereEvent::DistantSinging, focusPosition, 0.35f);
            break;
        case CharacterType::Jade:
            SymbolSystem::Instance().AdvanceDecoding(0.03f);
            if (npcLine.find("symbol") != std::string::npos || characterReply.find("symbol") != std::string::npos) {
                SymbolSystem::Instance().RevealSymbols(2.5f);
            }
            break;
        case CharacterType::Tabitha:
            TunnelMappingSystem::Instance().PlayDistortedEcho();
            break;
        case CharacterType::Victor:
            MemoryReplaySystem::Instance().ActivateMemoryMode(1.75f);
            MemoryReplaySystem::Instance().TriggerGhostReplica(npc.GetName(), characterReply, focusPosition);
            break;
        case CharacterType::Sara:
            MoralCorruptionSystem::Instance().OverhearConversation(npc.GetName(), npcLine + " | " + characterReply);
            if (npcLine.find("guilty") != std::string::npos || characterReply.find("sorry") != std::string::npos) {
                MoralCorruptionSystem::Instance().DetectGuilt(npc.GetName(), 0.1f);
            }
            break;
    }
}

std::string GetMonsterTauntLine(const Character& character, float distance, float worldClock) {
    static const std::array<std::string, 14> taunts = {
        "[MONSTER] We can hear your heart.",
        "[MONSTER] Come outside. We only want to talk.",
        "[MONSTER] That door will not save you forever.",
        "[MONSTER] You look tired.",
        "[MONSTER] We know where the others are.",
        "[MONSTER] The town told us your name.",
        "[MONSTER] Smile for us.",
        "[MONSTER] You cannot keep everyone safe.",
        "[MONSTER] The dark is already inside.",
        "[MONSTER] We remember you.",
        "[MONSTER] Open the window.",
        "[MONSTER] Someone is missing.",
        "[MONSTER] Your friends are calling from the woods.",
        "[MONSTER] We are closer than you think."
    };

    const std::size_t index = (static_cast<std::size_t>(character.GetType()) * 7 +
                               static_cast<std::size_t>(distance * 2.0f) +
                               static_cast<std::size_t>(worldClock)) % taunts.size();
    return taunts[index];
}

std::string GetCharacterMonsterResponseLine(const Character& character, float worldClock) {
    static const std::array<std::string, 5> lines = {
        "Boyd: Back up. I am not giving you anyone.",
        "Jade: You talk too much for something pretending to be human.",
        "Tabitha: I know what you are trying to do.",
        "Victor: I remember you. I remember what comes next.",
        "Sara: No. I am not listening anymore."
    };

    const std::size_t index = static_cast<std::size_t>(character.GetType()) % lines.size();
    (void)worldClock;
    return lines[index];
}

std::string GetMonsterScreamLine(int enemyCount, float distance) {
    static const std::array<std::string, 12> screams = {
        "[MONSTER] RAAAAAHHHHH!!!",
        "[MONSTER] SCREEEEEECH!!!",
        "[MONSTER] HOWWWWWW!!!",
        "[MONSTER] ROOOOOARRR!!!",
        "[MONSTER] GRRRRRAAAAHHH!!!",
        "[MONSTER] SHRIIIIEEEEK!!!",
        "[MONSTER] CRAAAAAASH!!!",
        "[MONSTER] WAAAAARGH!!!",
        "[MONSTER] KNOCK KNOCK.",
        "[MONSTER] LET US IN.",
        "[MONSTER] WE SEE YOU.",
        "[MONSTER] HIDE AND SEEK."
    };
    
    // Vary by enemy count and distance for some variety
    const std::size_t index = (static_cast<std::size_t>(enemyCount * 3) + static_cast<std::size_t>(distance * 2)) % screams.size();
    return screams[index];
}

void CreateColoredCubeMesh(Mesh& mesh, const glm::vec3& color) {
    const std::vector<MeshVertex> vertices = {
        // Front
        {glm::vec3(-0.5f, -0.5f,  0.5f), glm::vec3(0.0f, 0.0f, 1.0f), color},
        {glm::vec3( 0.5f, -0.5f,  0.5f), glm::vec3(0.0f, 0.0f, 1.0f), color},
        {glm::vec3( 0.5f,  0.5f,  0.5f), glm::vec3(0.0f, 0.0f, 1.0f), color},
        {glm::vec3(-0.5f,  0.5f,  0.5f), glm::vec3(0.0f, 0.0f, 1.0f), color},
        // Back
        {glm::vec3( 0.5f, -0.5f, -0.5f), glm::vec3(0.0f, 0.0f, -1.0f), color},
        {glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(0.0f, 0.0f, -1.0f), color},
        {glm::vec3(-0.5f,  0.5f, -0.5f), glm::vec3(0.0f, 0.0f, -1.0f), color},
        {glm::vec3( 0.5f,  0.5f, -0.5f), glm::vec3(0.0f, 0.0f, -1.0f), color},
        // Left
        {glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(-1.0f, 0.0f, 0.0f), color},
        {glm::vec3(-0.5f, -0.5f,  0.5f), glm::vec3(-1.0f, 0.0f, 0.0f), color},
        {glm::vec3(-0.5f,  0.5f,  0.5f), glm::vec3(-1.0f, 0.0f, 0.0f), color},
        {glm::vec3(-0.5f,  0.5f, -0.5f), glm::vec3(-1.0f, 0.0f, 0.0f), color},
        // Right
        {glm::vec3( 0.5f, -0.5f,  0.5f), glm::vec3(1.0f, 0.0f, 0.0f), color},
        {glm::vec3( 0.5f, -0.5f, -0.5f), glm::vec3(1.0f, 0.0f, 0.0f), color},
        {glm::vec3( 0.5f,  0.5f, -0.5f), glm::vec3(1.0f, 0.0f, 0.0f), color},
        {glm::vec3( 0.5f,  0.5f,  0.5f), glm::vec3(1.0f, 0.0f, 0.0f), color},
        // Top
        {glm::vec3(-0.5f,  0.5f,  0.5f), glm::vec3(0.0f, 1.0f, 0.0f), color},
        {glm::vec3( 0.5f,  0.5f,  0.5f), glm::vec3(0.0f, 1.0f, 0.0f), color},
        {glm::vec3( 0.5f,  0.5f, -0.5f), glm::vec3(0.0f, 1.0f, 0.0f), color},
        {glm::vec3(-0.5f,  0.5f, -0.5f), glm::vec3(0.0f, 1.0f, 0.0f), color},
        // Bottom
        {glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(0.0f, -1.0f, 0.0f), color},
        {glm::vec3( 0.5f, -0.5f, -0.5f), glm::vec3(0.0f, -1.0f, 0.0f), color},
        {glm::vec3( 0.5f, -0.5f,  0.5f), glm::vec3(0.0f, -1.0f, 0.0f), color},
        {glm::vec3(-0.5f, -0.5f,  0.5f), glm::vec3(0.0f, -1.0f, 0.0f), color}
    };

    const std::vector<unsigned int> indices = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4,
        8, 9, 10, 10, 11, 8,
        12, 13, 14, 14, 15, 12,
        16, 17, 18, 18, 19, 16,
        20, 21, 22, 22, 23, 20
    };

    mesh.Create(vertices, indices);
}

void CreateQuestTriangleMesh(Mesh& mesh, const glm::vec3& color) {
    const std::vector<MeshVertex> vertices = {
        // Front triangle
        {glm::vec3(0.0f, 1.3f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), color},
        {glm::vec3(-0.9f, -0.8f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), color},
        {glm::vec3(0.9f, -0.8f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), color},
        // Back triangle
        {glm::vec3(0.0f, 1.3f, 0.14f), glm::vec3(0.0f, 0.0f, -1.0f), color},
        {glm::vec3(-0.9f, -0.8f, 0.14f), glm::vec3(0.0f, 0.0f, -1.0f), color},
        {glm::vec3(0.9f, -0.8f, 0.14f), glm::vec3(0.0f, 0.0f, -1.0f), color}
    };

    const std::vector<unsigned int> indices = {
        0, 1, 2,
        3, 5, 4,
        0, 3, 4, 4, 1, 0,
        1, 4, 5, 5, 2, 1,
        2, 5, 3, 3, 0, 2
    };

    mesh.Create(vertices, indices);
}

void RenderCharacterCube(
    Shader& shader,
    const Mesh& mesh,
    const Camera& camera,
    float aspectRatio,
    const glm::vec3& position,
    const glm::vec3& scale,
    const glm::vec3& tint) {
    const glm::mat4 projection = camera.GetProjectionMatrix(aspectRatio);
    const glm::mat4 view = camera.GetViewMatrix();
    const glm::mat4 model = glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), scale);

    shader.Bind();
    shader.SetMat4("projection", projection);
    shader.SetMat4("view", view);
    shader.SetMat4("model", model);
    shader.SetVec3("color", tint);
    mesh.Draw();
    shader.Unbind();
}

glm::vec3 FindNearestEnemyPosition(const std::vector<Enemy>& enemies, const glm::vec3& npcPosition, float sightRange, bool& visibleOut) {
    visibleOut = false;
    if (enemies.empty()) {
        return npcPosition;
    }

    float bestDistance = sightRange;
    glm::vec3 bestPosition = npcPosition;

    for (const Enemy& enemy : enemies) {
        glm::vec3 delta = enemy.transform.position - npcPosition;
        delta.y = 0.0f;
        const float distance = glm::length(delta);
        if (distance < bestDistance) {
            bestDistance = distance;
            bestPosition = enemy.transform.position;
            visibleOut = true;
        }
    }

    return bestPosition;
}

void InitializeModels() {
    // ---- Character mesh (small model, load synchronously) ----
    if (!gPlayerReady) {
        std::vector<MeshVertex> vertices;
        std::vector<unsigned int> indices;
        if (Loader::LoadOBJ("assets/models/boyd.obj", vertices, indices)) {
            gPlayerMesh.Create(vertices, indices);
            gPlayerShader.Load("assets/shaders/model_lit.vert", "assets/shaders/model_lit.frag");
            gPlayerReady = gPlayerMesh.IsValid();
        } else {
            std::cerr << "Failed to load character OBJ: assets/models/boyd.obj\n";
            CreateColoredCubeMesh(gPlayerMesh, glm::vec3(0.85f, 0.85f, 0.95f));
            gPlayerShader.Load("assets/shaders/model_lit.vert", "assets/shaders/model_lit.frag");
            gPlayerReady = gPlayerMesh.IsValid();
        }
    }

    // ---- Map (Resident evil model, load synchronously for immediate display) ----
    static bool gVillageReady = false;
    gVillageReady = false;

    if (!gCharacterReady) {
        CreateColoredCubeMesh(gNpcMesh, glm::vec3(0.25f, 0.90f, 0.65f));
        CreateColoredCubeMesh(gEnemyMesh, glm::vec3(0.95f, 0.22f, 0.18f));
        CreateQuestTriangleMesh(gQuestMarkerMesh, glm::vec3(1.0f, 0.1f, 0.1f));
        gCharacterShader.Load("assets/shaders/model_lit.vert", "assets/shaders/model_lit.frag");
        gCharacterReady = gNpcMesh.IsValid() && gEnemyMesh.IsValid();
        gQuestMarkerReady = gQuestMarkerMesh.IsValid();
    }
}
}

World::World() {
    entityManager.BindStorage(&characters, &npcs, &enemies, &activeCharacterIndex);
}

void World::Initialize() {
    // --- Terrain project: load building meshes, doors, building collision ---
    player.transform.position = glm::vec3(0.0f, 2.0f, 10.0f);
    LoadBuildingMeshes(&collisionWorld, doors);
    player.SetCollisionWorld(&collisionWorld);
    std::cout << "[World] Buildings loaded (" << doors.size() << " doors).\n";

    InitializeModels();
    InitializeCharacters();
    InitializePlaceholderWorldRules();

    // Create and initialize quest system with pointer to worldClock
    questSystem = std::make_unique<QuestSystem>(&worldClock);
    interactionSystem.Initialize();
    puzzleManager.Initialize();
    audioManager = std::make_unique<AudioManager>();
    if (audioManager->Initialize()) {
        const std::array<std::pair<const char*, const char*>, 10> cueFiles = {{
            {"puzzle_tick", "assets/audio/sfx/puzzle_tick.wav"},
            {"puzzle_complete", "assets/audio/sfx/puzzle_complete.wav"},
            {"puzzle_fail", "assets/audio/sfx/puzzle_fail.wav"},
            {"player_hurt", "assets/audio/sfx/characters/player_hurt.wav"},
            {"footstep_wood", "assets/audio/sfx/footsteps/footstep_wood.wav"},
            {"footstep_dirt", "assets/audio/sfx/footsteps/footstep_dirt.wav"},
            {"footstep_stone", "assets/audio/sfx/footsteps/footstep_stone.wav"},
            {"ambient_tension_low", "assets/audio/music/ambient_tension_low.wav"},
            {"ambient_tension_high", "assets/audio/music/ambient_tension_high.wav"},
            {"cinematic_event_sting", "assets/audio/music/cinematic_event_sting.wav"}
        }};

        for (const auto& [cueName, relativePath] : cueFiles) {
            if (std::filesystem::exists(relativePath)) {
                audioManager->LoadSound(cueName, relativePath);
            } else {
                std::cout << "[Audio] Missing cue file: " << relativePath << "\n";
            }
        }

        InitializeVoiceCueLibrary(*audioManager);
        audioManager->PlaySound("ambient_tension_low", 0.30f);
    }

    puzzleManager.SetSoundHook([this](const std::string& cue) {
        bool played = false;
        if (audioManager) {
            played = audioManager->PlaySound(cue);
        }
        if (!played) {
            std::cout << "[AudioCue] " << cue << "\n";
        }
    });
    puzzleManager.SetConsequenceHook([this](const std::string& consequence) {
        ApplyPuzzleConsequence(consequence);
    });
    puzzleManager.SetCompletionCallback([this](CharacterType characterType, int objectiveIndex) {
        eventBus.Publish(PuzzleCompletedEvent{characterType, objectiveIndex});

        if (questSystem) {
            questSystem->AdvanceObjective(characterType, objectiveIndex);
        }
        StoryManager::Instance().AdvanceStep(objectiveIndex);
        interactionSystem.MarkQuestStepSolved(characterType, objectiveIndex);

        const Quest* questForHooks = questSystem ? questSystem->GetCharacterQuest(characterType) : nullptr;
        const glm::vec3 focusPosition = (activeCharacterIndex >= 0 && activeCharacterIndex < static_cast<int>(characters.size()))
            ? characters[activeCharacterIndex]->transform.position
            : glm::vec3(0.0f);
        TriggerQuestlineNarrativeHooks(characterType, objectiveIndex, questForHooks, focusPosition);

        // Ensure any story flags on nodes for this objective are set so subsequent nodes unlock
        if (questSystem) {
            const auto& nodes = interactionSystem.GetNodes();
            for (const auto& n : nodes) {
                if (n.questObjectiveIndex == objectiveIndex &&
                    (n.requiredCharacter.empty() || n.requiredCharacter == CharacterTypeToName(characterType))) {
                    if (!n.questFlag.empty()) {
                        questSystem->SetStoryFlag(n.questFlag);
                        std::cout << "[World] Set story flag from node '" << n.id << "' -> '" << n.questFlag << "'\n";
                    }
                }
            }
        }

        // Spawn a visible checkpoint flag after each puzzle completion to return to later
        InteractionNode checkpoint;
        std::ostringstream idstream;
        idstream << "checkpoint_" << static_cast<int>(characterType) << "_" << objectiveIndex;
        checkpoint.id = idstream.str();
        checkpoint.type = InteractionType::Trigger;
        checkpoint.name = "Checkpoint Flag";
        // Place at current active character position if available
        if (activeCharacterIndex >= 0 && activeCharacterIndex < static_cast<int>(characters.size())) {
            checkpoint.position = characters[activeCharacterIndex]->transform.position + glm::vec3(0.0f, 0.0f, 0.0f);
        } else {
            checkpoint.position = glm::vec3(0.0f);
        }
        checkpoint.radius = 2.0f;
        checkpoint.prompt = "[Checkpoint]";
        
        // Create objective-specific messages
        std::string checkpointMessage = "=== CHECKPOINT SAVED ===\n";
        std::string feedbackMsg = "◆ CHECKPOINT SAVED!";
        
        if (objectiveIndex == 0) {
            checkpointMessage += "\nNEXT OBJECTIVE: Gather 3 pieces of evidence\n\nUse COMPASS (top-left) to find items:\n• BLOODY KNIFE\n• CULT LEDGER\n• STRANGE IDOL\n\nPress E to inspect each item.\nThen return to EVIDENCE BOARD.";
            feedbackMsg = "◆ CHECKPOINT SAVED! Follow COMPASS to gather evidence.";
        } else if (objectiveIndex == 1) {
            checkpointMessage += "\nNEXT OBJECTIVE: Discover cult gathering place\n\nUse COMPASS (top-left) to locate the\nCULT TRAIL MARKER where the ritual\nceremony takes place.\n\nPress E to investigate.";
            feedbackMsg = "◆ CHECKPOINT SAVED! Follow COMPASS to cult gathering place.";
        } else if (objectiveIndex == 2) {
            checkpointMessage += "\nNEXT OBJECTIVE: Confront the leader\n\nUse COMPASS (top-left) to find\nLEADER CLUE.\n\nPress E to trigger the confrontation puzzle.";
            feedbackMsg = "◆ CHECKPOINT SAVED! Follow COMPASS to LEADER CLUE.";
        } else if (objectiveIndex == 3) {
            checkpointMessage += "\nNEXT OBJECTIVE: Prevent the ritual\n\nUse COMPASS (top-left) to reach\nRITUAL CIRCLE.\n\nPress E to disrupt the ritual.";
            feedbackMsg = "◆ CHECKPOINT SAVED! Follow COMPASS to RITUAL CIRCLE.";
        } else {
            checkpointMessage += "\nNEXT OBJECTIVE: Continue investigation\n\nUse COMPASS (top-left) for guidance.\nFollow the waypoint and press E.";
            feedbackMsg = "◆ CHECKPOINT SAVED! Follow COMPASS waypoint.";
        }
        
        checkpoint.successMessage = checkpointMessage;
        checkpoint.questFlag = "";
        checkpoint.requiredFlag = "";
        checkpoint.questObjectiveIndex = -1;
        checkpoint.requiredCharacter = "";
        interactionSystem.AddNode(checkpoint);
        std::cout << "[World] Spawned checkpoint: " << checkpoint.id << " at " << checkpoint.position.x << "," << checkpoint.position.y << "," << checkpoint.position.z << "\n";
        lastInteractionFeedback = feedbackMsg;
        lastInteractionFeedbackTimer.Start(5.0f);

        const Quest* quest = questSystem ? questSystem->GetCharacterQuest(characterType) : nullptr;
        if (quest && quest->IsComplete()) {
            hasActiveQuest = false;
            lastInteractionFeedback = "Final clue solved. The conspiracy unravels.";
            lastInteractionFeedbackTimer.Start(3.0f);
        }
    });

    SeedNarrativeSystems();
    
    // Convert character pointers for quest system
    std::array<Character*, 5> charPtrs = {
        characters[0].get(),
        characters[1].get(),
        characters[2].get(),
        characters[3].get(),
        characters[4].get()
    };
    questSystem->Initialize(charPtrs);
    
    // Assign quests to characters
    for (size_t i = 0; i < characters.size(); ++i) {
        characters[i]->SetQuest(questSystem->GetCharacterQuest(characters[i]->GetType()));
    }

    // Building collision is already loaded above via LoadBuildingMeshes.
    collisionWorld.BuildBVH();

    // Wire collision system to all characters
    entityManager.BindCollisionWorld(&collisionWorld);
    
    // Activate the first character (Boyd)
    if (!characters.empty()) {
        characters[activeCharacterIndex]->OnSwitchedTo();
    }

    initialSpawnState = CaptureSaveState();
    hasInitialSpawnState = true;
}


void World::Update(const Camera& camera, float dt) {
    (void)camera;

    UpdateWorldSystemsPhase(dt);

    if (puzzleManager.IsActive()) {
        if (questSystem) {
            questSystem->Update(dt);
        }
        return;
    }

    UpdateTimersAndCooldownsPhase(dt);

    UpdateTimeOfDay(dt);

    UpdateQuestAndInteractionPhase(dt);

    // Update active character
    Character* activeChar = GetActiveCharacter();
    if (activeChar) {
        activeChar->Update(dt);
    }

    float playerSound = 0.0f;
    float playerLight = nightTime ? 0.42f : 0.10f;
    if (activeChar) {
        if (hasPreviousActivePosition && dt > 0.001f) {
            const float moved = HorizontalDistance(activeChar->transform.position, previousActivePosition);
            const float speed = moved / dt;
            playerSound = Clamp01(speed / 9.0f);
        }

        if (activeChar->IsSprinting()) {
            playerSound = std::max(playerSound, 0.82f);
        } else if (activeChar->IsCrouching()) {
            playerSound *= 0.28f;
            playerLight *= 0.70f;
        }
    }
    
    UpdateOffscreenCharacters(dt);
    
    // Debug: Print active character position every 120 frames
    debugFrameCounter++;
    if (debugFrameCounter >= 120 && activeChar) {
        const auto& pos = activeChar->transform.position;
        std::cout << "[" << activeChar->GetName() << "] Pos: x=" << pos.x << " y=" << pos.y << " z=" << pos.z 
                  << " | Health: " << activeChar->GetHealth() << "\n";
        debugFrameCounter = 0;
    }

    std::vector<bool> npcThreatened(npcs.size(), false);
    glm::vec3 rescueTarget = glm::vec3(0.0f);
    bool hasRescueTarget = false;

    for (std::size_t i = 0; i < npcs.size(); ++i) {
        NPC& npc = npcs[i];
        npc.SetNight(nightTime);
        bool enemyVisible = false;
        const glm::vec3 threatPosition = FindNearestEnemyPosition(enemies, npc.transform.position, kNpcSightRange, enemyVisible);
        npc.SetThreatPosition(threatPosition, enemyVisible);
        npcThreatened[i] = enemyVisible || npc.IsInDanger();
        if (npcThreatened[i] && !hasRescueTarget) {
            rescueTarget = npc.transform.position;
            hasRescueTarget = true;
        }
    }

    for (std::size_t i = 0; i < npcs.size(); ++i) {
        NPC& npc = npcs[i];
        const bool canRescue = hasRescueTarget && !npcThreatened[i] && HorizontalDistance(npc.transform.position, rescueTarget) <= 12.0f;
        npc.SetRescueTarget(rescueTarget, canRescue);
        npc.Update(dt);
    }

    if (!playerKilled && activeChar) {
        for (Enemy& enemy : enemies) {
            EnemyPerception bestPerception = EvaluateTargetStimulus(
                enemy.transform.position,
                activeChar->transform.position,
                kEnemySightRange,
                kEnemyHearingRange,
                kEnemyLightRange,
                playerSound,
                playerLight);
            bestPerception.playerTarget = true;
            if (IsProtectedByShelter(activeChar->transform.position)) {
                // If the active character is sheltered, set a sheltered perception so monsters
                // can approach and torment the shelter edge rather than enter it.
                const glm::vec3 shelterCenter = GetNearestShelterCenter(activeChar->transform.position);
                glm::vec3 dir = shelterCenter - enemy.transform.position;
                dir.y = 0.0f;
                if (glm::length(dir) > 0.001f) dir = glm::normalize(dir);
                const ShelterZone* rz = nullptr;
                for (const ShelterZone& s : shelterZones) {
                    if (HorizontalDistance(activeChar->transform.position, s.center) <= s.radius) { rz = &s; break; }
                }
                glm::vec3 edgePos = shelterCenter;
                if (rz) edgePos = shelterCenter + dir * (rz->radius + 0.8f);
                bestPerception.hasTarget = true;
                bestPerception.visible = false;
                bestPerception.sound = 0.0f;
                bestPerception.light = 0.0f;
                bestPerception.proximity = Clamp01(1.0f - (HorizontalDistance(enemy.transform.position, edgePos) / kEnemySightRange));
                bestPerception.targetPosition = edgePos;
                bestPerception.targetSheltered = true;
            }

            float bestScore = ScorePerception(bestPerception);

            for (std::size_t i = 0; i < characters.size(); ++i) {
                if (static_cast<int>(i) == activeCharacterIndex) {
                    continue;
                }

                const Character& offscreenCharacter = *characters[i];
                if (IsProtectedByShelter(offscreenCharacter.transform.position)) {
                    // Turn sheltered offscreen characters into sheltered perceptions
                    const glm::vec3 shelterCenter = GetNearestShelterCenter(offscreenCharacter.transform.position);
                    glm::vec3 dir = shelterCenter - enemy.transform.position;
                    dir.y = 0.0f;
                    if (glm::length(dir) > 0.001f) dir = glm::normalize(dir);
                    const ShelterZone* rz = nullptr;
                    for (const ShelterZone& s : shelterZones) {
                        if (HorizontalDistance(offscreenCharacter.transform.position, s.center) <= s.radius) { rz = &s; break; }
                    }
                    glm::vec3 edgePos = shelterCenter;
                    if (rz) edgePos = shelterCenter + dir * (rz->radius + 0.8f);
                    const EnemyPerception characterPerception = EvaluateTargetStimulus(
                        enemy.transform.position,
                        edgePos,
                        kEnemySightRange,
                        kEnemyHearingRange,
                        kEnemyLightRange,
                        0.18f,
                        0.32f);
                    EnemyPerception shelved = characterPerception;
                    shelved.hasTarget = true;
                    shelved.visible = false;
                    shelved.playerTarget = true;
                    shelved.targetPosition = edgePos;
                    shelved.targetSheltered = true;
                    const float characterScore = ScorePerception(shelved);
                    if (!bestPerception.hasTarget || characterScore > bestScore) {
                        bestPerception = shelved;
                        bestScore = characterScore;
                    }
                    continue;
                }

                const float offscreenSound = nightTime ? 0.18f : 0.24f;
                const float offscreenLight = nightTime ? 0.32f : 0.08f;
                const EnemyPerception characterPerception = EvaluateTargetStimulus(
                    enemy.transform.position,
                    offscreenCharacter.transform.position,
                    kEnemySightRange,
                    kEnemyHearingRange,
                    kEnemyLightRange,
                    offscreenSound,
                    offscreenLight);

                const float characterScore = ScorePerception(characterPerception);
                if (characterPerception.hasTarget && (!bestPerception.hasTarget || characterScore > bestScore)) {
                    bestPerception = characterPerception;
                    bestScore = characterScore;
                }
            }

            for (const NPC& npc : npcs) {
                if (IsProtectedByShelter(npc.transform.position)) {
                    const glm::vec3 shelterCenter = GetNearestShelterCenter(npc.transform.position);
                    glm::vec3 dir = shelterCenter - enemy.transform.position;
                    dir.y = 0.0f;
                    if (glm::length(dir) > 0.001f) dir = glm::normalize(dir);
                    const ShelterZone* rz = nullptr;
                    for (const ShelterZone& s : shelterZones) {
                        if (HorizontalDistance(npc.transform.position, s.center) <= s.radius) { rz = &s; break; }
                    }
                    glm::vec3 edgePos = shelterCenter;
                    if (rz) edgePos = shelterCenter + dir * (rz->radius + 0.8f);
                    const float npcSound = npc.IsInDanger() ? 0.78f : (nightTime ? 0.16f : 0.28f);
                    const float npcLight = nightTime ? 0.35f : 0.08f;
                    const EnemyPerception npcPerception = EvaluateTargetStimulus(
                        enemy.transform.position,
                        edgePos,
                        kEnemySightRange,
                        kEnemyHearingRange,
                        kEnemyLightRange,
                        npcSound,
                        npcLight);
                    EnemyPerception shelved = npcPerception;
                    shelved.hasTarget = true;
                    shelved.visible = false;
                    shelved.playerTarget = false;
                    shelved.targetPosition = edgePos;
                    shelved.targetSheltered = true;
                    const float npcScore = ScorePerception(shelved);
                    if (npcPerception.hasTarget && (!bestPerception.hasTarget || npcScore > bestScore)) {
                        bestPerception = shelved;
                        bestScore = npcScore;
                    }
                    continue;
                }

                const float npcSound = npc.IsInDanger() ? 0.78f : (nightTime ? 0.16f : 0.28f);
                const float npcLight = nightTime ? 0.35f : 0.08f;
                const EnemyPerception npcPerception = EvaluateTargetStimulus(
                    enemy.transform.position,
                    npc.transform.position,
                    kEnemySightRange,
                    kEnemyHearingRange,
                    kEnemyLightRange,
                    npcSound,
                    npcLight);

                const float npcScore = ScorePerception(npcPerception);
                if (npcPerception.hasTarget && (!bestPerception.hasTarget || npcScore > bestScore)) {
                    bestPerception = npcPerception;
                    bestScore = npcScore;
                }
            }

            enemy.SetPerception(bestPerception);
            enemy.Update(dt);
            
            if (nightTime) {
                // Apply damage to any character in attack range (with cooldown to prevent spam)
                for (std::size_t charIdx = 0; charIdx < characters.size(); ++charIdx) {
                    Character& character = *characters[charIdx];
                    if (enemy.IsInAttackRange(character.transform.position) && characterDamageCooldowns[charIdx] <= 0.0f) {
                        // Apply damage to this character
                        const float damageAmount = 15.0f;
                        character.TakeDamage(damageAmount);
                        characterDamageCooldowns[charIdx] = 1.5f;  // 1.5 second cooldown between hits
                        
                        // Update HUD feedback if this is the active character
                        if (static_cast<int>(charIdx) == activeCharacterIndex) {
                            lastDamageAmount = damageAmount;
                            lastDamageDisplayTimer.Start(kDamageDisplayDuration);
                            if (audioManager) {
                                audioManager->PlaySound("player_hurt", 0.90f);
                            }
                            
                            // Trigger monster scream
                            const float distance = HorizontalDistance(character.transform.position, enemy.transform.position);
                            lastMonsterScream = GetMonsterScreamLine(static_cast<int>(enemies.size()), distance);
                            monsterScreamDisplayTimer.Start(kScreamDisplayDuration);
                            PlayVoiceLine(audioManager.get(), "monster", lastMonsterScream, 0.95f);
                        }
                        
                        std::cout << "[Damage] " << character.GetName() << " takes " << damageAmount << " damage! HP: " << character.GetHealth() << "\n";
                        if (static_cast<int>(charIdx) == activeCharacterIndex) {
                            std::cout << lastMonsterScream << "\n";
                        }
                    }
                }
            }
        }
    }
    
    // Update monster scream and damage display timers
    monsterScreamDisplayTimer.Tick(dt);
    lastDamageDisplayTimer.Tick(dt);
    npcDialogueDisplayTimer.Tick(dt);

    if (activeChar) {
        previousActivePosition = activeChar->transform.position;
        hasPreviousActivePosition = true;
    }

    for (auto& character : characters) {
        character->transform.position = ClampArenaPosition(character->transform.position);
    }
    for (NPC& npc : npcs) {
        npc.transform.position = ClampArenaPosition(npc.transform.position);
    }
    for (Enemy& enemy : enemies) {
        enemy.transform.position = ClampArenaPosition(enemy.transform.position);
    }

    if (activeChar && activeCharacterIndex >= 0 && activeCharacterIndex < static_cast<int>(characters.size())) {
        const std::size_t activeIndex = static_cast<std::size_t>(activeCharacterIndex);
        for (std::size_t npcIndex = 0; npcIndex < npcs.size(); ++npcIndex) {
            NPC& npc = npcs[npcIndex];
            const float distance = HorizontalDistance(activeChar->transform.position, npc.transform.position);
            if (distance < 2.75f && npcTalkCooldowns[npcIndex][activeIndex] <= 0.0f) {
                const std::string npcLine = GetNpcLine(npc, *activeChar, worldClock);
                const std::string characterReply = GetCharacterNpcReplyLine(npc, *activeChar, worldClock);
                std::ostringstream dialogue;
                dialogue << npc.GetName() << ": " << npcLine << " | " << activeChar->GetName() << ": " << characterReply;
                lastNpcDialogue = dialogue.str();
                npcDialogueDisplayTimer.Start(kNpcDialogueDisplayDuration);
                
                std::cout << "[Talk] " << activeChar->GetName() << " and " << lastNpcDialogue << "\n";
                const std::string npcVoiceGroup = IsFemaleSpeaker(npc.GetName()) ? "npc_female" : "npc_male";
                PlayVoiceLine(audioManager.get(), npcVoiceGroup, npcLine, 0.84f);
                PlayVoiceLine(audioManager.get(), "responses", activeChar->GetName() + ": " + characterReply, 0.80f);
                npcTalkCooldowns[npcIndex][activeIndex] = 5.0f;
            }

            if (npc.IsInDanger() && npcPanicCooldowns[npcIndex] <= 0.0f) {
                const std::string panicLine = GetPanicLine(*activeChar);
                std::ostringstream panic;
                panic << npc.GetName() << ": " << panicLine;
                lastNpcDialogue = panic.str();
                npcDialogueDisplayTimer.Start(kNpcDialogueDisplayDuration);
                
                std::cout << "[Panic] " << lastNpcDialogue << "\n";
                const std::string panicGroup = IsFemaleSpeaker(npc.GetName()) ? "panic_female" : "panic_male";
                PlayVoiceLine(audioManager.get(), panicGroup, panicLine, 0.95f);
                npcPanicCooldowns[npcIndex] = 4.5f;
            }
        }

        for (std::size_t characterIndex = 0; characterIndex < characters.size(); ++characterIndex) {
            if (static_cast<int>(characterIndex) == activeCharacterIndex) {
                continue;
            }

            Character& otherCharacter = *characters[characterIndex];
            const float distance = HorizontalDistance(activeChar->transform.position, otherCharacter.transform.position);
            if (distance < 2.2f && characterReactionCooldowns[characterIndex] <= 0.0f) {
                lastNpcDialogue = GetCharacterPairLine(*activeChar, otherCharacter, worldClock);
                npcDialogueDisplayTimer.Start(kNpcDialogueDisplayDuration);
                std::cout << "[Conversation] " << lastNpcDialogue << "\n";
                characterReactionCooldowns[characterIndex] = 6.0f;
            }
        }

        if (nightTime) {
            for (const Enemy& enemy : enemies) {
                const float distance = HorizontalDistance(activeChar->transform.position, enemy.transform.position);
                if (distance < 8.0f && characterMonsterInteractionCooldowns[activeIndex] <= 0.0f) {
                    lastMonsterScream = GetMonsterTauntLine(*activeChar, distance, worldClock);
                    monsterScreamDisplayTimer.Start(kScreamDisplayDuration);
                    lastNpcDialogue = GetCharacterMonsterResponseLine(*activeChar, worldClock);
                    npcDialogueDisplayTimer.Start(kNpcDialogueDisplayDuration);
                    std::cout << lastMonsterScream << "\n";
                    std::cout << "[Monster Response] " << lastNpcDialogue << "\n";
                    PlayVoiceLine(audioManager.get(), "monster", lastMonsterScream, 0.95f);
                    PlayVoiceLine(audioManager.get(), "responses", lastNpcDialogue, 0.88f);
                    characterMonsterInteractionCooldowns[activeIndex] = 7.5f;
                    break;
                }

                if (distance < 6.5f && characterReactionCooldowns[activeCharacterIndex] <= 0.0f) {
                    lastNpcDialogue = activeChar->GetName() + ": " + GetPanicLine(*activeChar);
                    npcDialogueDisplayTimer.Start(kNpcDialogueDisplayDuration);
                    std::cout << "[Panic] " << lastNpcDialogue << "\n";
                    const std::string activePanicGroup = IsFemaleSpeaker(activeChar->GetName()) ? "panic_female" : "panic_male";
                    PlayVoiceLine(audioManager.get(), activePanicGroup, GetPanicLine(*activeChar), 0.95f);
                    characterReactionCooldowns[activeCharacterIndex] = 4.0f;
                    break;
                }
            }
        }
    }

    for (std::size_t npcIndex = 0; npcIndex < npcs.size(); ++npcIndex) {
        if (npcNeighborTalkCooldowns[npcIndex] > 0.0f) {
            continue;
        }

        for (std::size_t otherNpcIndex = npcIndex + 1; otherNpcIndex < npcs.size(); ++otherNpcIndex) {
            if (HorizontalDistance(npcs[npcIndex].transform.position, npcs[otherNpcIndex].transform.position) < 3.25f) {
                lastNpcDialogue = GetNpcNeighborLine(npcs[npcIndex], npcs[otherNpcIndex], worldClock);
                npcDialogueDisplayTimer.Start(kNpcDialogueDisplayDuration);
                npcNeighborTalkCooldowns[npcIndex] = 9.0f;
                npcNeighborTalkCooldowns[otherNpcIndex] = 9.0f;
                std::cout << "[NPC Chat] " << lastNpcDialogue << "\n";
                const std::string speaker = GetSpeakerName(lastNpcDialogue);
                const std::string neighborVoiceGroup = IsFemaleSpeaker(speaker) ? "npc_female" : "npc_male";
                PlayVoiceLine(audioManager.get(), neighborVoiceGroup, lastNpcDialogue, 0.80f);
                // mark the NPCs as conversing for a short time
                npcs[npcIndex].StartConversation(4.0f);
                npcs[otherNpcIndex].StartConversation(4.0f);
                break;
            }
        }
    }

    // Update doors from terrain project
    for (auto& door : doors) {
        door.Update(dt);
    }
}

void World::UpdateWorldSystemsPhase(float dt) {
    worldManager.UpdateWorldSimulation(dt);
}

void World::UpdateTimersAndCooldownsPhase(float dt) {
    EnsureNpcDialogueCooldowns();

    for (float& cooldown : characterReactionCooldowns) {
        cooldown = std::max(0.0f, cooldown - dt);
    }
    for (auto& npcCooldowns : npcTalkCooldowns) {
        for (float& cooldown : npcCooldowns) {
            cooldown = std::max(0.0f, cooldown - dt);
        }
    }
    for (float& cooldown : npcPanicCooldowns) {
        cooldown = std::max(0.0f, cooldown - dt);
    }
    for (float& cooldown : npcNeighborTalkCooldowns) {
        cooldown = std::max(0.0f, cooldown - dt);
    }
    for (float& cooldown : characterDamageCooldowns) {
        cooldown = std::max(0.0f, cooldown - dt);
    }
    for (float& cooldown : characterMonsterInteractionCooldowns) {
        cooldown = std::max(0.0f, cooldown - dt);
    }

    lastInteractionFeedbackTimer.Tick(dt);
}

void World::UpdateQuestAndInteractionPhase(float dt) {
    if (!questSystem) {
        return;
    }

    // If it's night and the active character is exposed (not inside a talisman shelter),
    // pause quest progression and cancel any active puzzles until morning.
    Character* activeChar = GetActiveCharacter();
    if (nightTime && activeChar && !IsProtectedByShelter(activeChar->transform.position)) {
        if (puzzleManager.IsActive()) {
            puzzleManager.CancelActivePuzzle();
        }
        // Skip quest updates while exposed at night
        return;
    }

    questSystem->Update(dt);

    if (tabithaWhisperTempRouteTimer > 0.0f) {
        tabithaWhisperTempRouteTimer = std::max(0.0f, tabithaWhisperTempRouteTimer - dt);
        if (tabithaWhisperTempRouteTimer <= 0.0f) {
            questSystem->ClearStoryFlag("tabitha_whisper_hidden_path_open");
            std::cout << "[World] Tabitha's temporary whisper route faded.\n";
        }
    }

    if (tabithaWhisperFalseChamberTimer > 0.0f) {
        tabithaWhisperFalseChamberTimer = std::max(0.0f, tabithaWhisperFalseChamberTimer - dt);
        if (tabithaWhisperFalseChamberTimer <= 0.0f) {
            questSystem->ClearStoryFlag("tabitha_whisper_false_chamber_active");
            std::cout << "[World] Tabitha's false chamber dissolved.\n";
        }
    }

    EnsureTabithaWhisperRouteNodes();

    while (questSystem->HasPendingConsequences()) {
        const std::string& consequence = questSystem->GetNextConsequence();
        std::cout << "[Story Consequence] " << consequence << "\n";
    }

    interactionSystem.Update(dt, *questSystem);

    if (hasActiveQuest && !characters.empty()) {
        Quest* activeQuest = questSystem->GetCharacterQuest(activeQuestCharacter);
        if (activeQuest && !activeQuest->IsComplete() && !activeQuest->IsFailed()) {
            const int nextObjectiveIndex = activeQuest->GetNextIncompleteObjectiveIndex();
            const auto& objectives = activeQuest->GetObjectives();
            if (nextObjectiveIndex >= 0 && nextObjectiveIndex < static_cast<int>(objectives.size())) {
                const QuestObjective& currentObjective = objectives[nextObjectiveIndex];
                if (currentObjective.type == ObjectiveType::Combat) {
                    const std::string requiredCharacter = CharacterTypeToName(activeQuestCharacter);
                    const Character* activeCharForSpawn = GetActiveCharacter();
                    if (activeCharForSpawn) {
                        float closestMatchingNodeDistance = std::numeric_limits<float>::max();
                        bool hasMatchingNode = false;
                        for (const InteractionNode& node : interactionSystem.GetNodes()) {
                            if (node.questObjectiveIndex != nextObjectiveIndex || node.requiredCharacter != requiredCharacter) {
                                continue;
                            }
                            if (!node.active && node.type != InteractionType::Door) {
                                continue;
                            }
                            if (!node.requiredFlag.empty() && !questSystem->HasStoryFlag(node.requiredFlag)) {
                                continue;
                            }
                            const float distance = HorizontalDistance(activeCharForSpawn->transform.position, node.position);
                            hasMatchingNode = true;
                            closestMatchingNodeDistance = std::min(closestMatchingNodeDistance, distance);
                        }

                        if (!hasMatchingNode || closestMatchingNodeDistance > 5.0f) {
                            const std::string runtimeNodeId = "runtime_combat_" + requiredCharacter + "_" + std::to_string(nextObjectiveIndex);
                            bool nodeAlreadySpawned = false;
                            for (const InteractionNode& node : interactionSystem.GetNodes()) {
                                if (node.id == runtimeNodeId) {
                                    nodeAlreadySpawned = true;
                                    break;
                                }
                            }

                            if (!nodeAlreadySpawned) {
                                InteractionNode runtimeNode;
                                runtimeNode.id = runtimeNodeId;
                                runtimeNode.type = InteractionType::Trigger;
                                runtimeNode.name = currentObjective.description;
                                runtimeNode.position = activeCharForSpawn->transform.position + glm::vec3(2.0f, 0.0f, 0.0f);
                                runtimeNode.radius = 3.0f;
                                runtimeNode.prompt = "Confront";
                                runtimeNode.successMessage = "The cult leader appears. Press E to confront him.";
                                runtimeNode.questFlag = "runtime_combat_start_" + requiredCharacter;
                                runtimeNode.requiredFlag = "";
                                runtimeNode.questObjectiveIndex = nextObjectiveIndex;
                                runtimeNode.requiredCharacter = requiredCharacter;
                                interactionSystem.AddNode(runtimeNode);
                                std::cout << "[World] Spawned runtime combat encounter: " << runtimeNode.id << " at "
                                          << runtimeNode.position.x << "," << runtimeNode.position.y << "," << runtimeNode.position.z << "\n";
                            }
                        }
                    }
                }
            }
        }
    }

    if (hasActiveQuest) {
        const Quest* activeQuest = questSystem->GetCharacterQuest(activeQuestCharacter);
        if (activeQuest && activeQuest->IsComplete()) {
            hasActiveQuest = false;
            lastInteractionFeedback = "Quest complete! You can start another one.";
            lastInteractionFeedbackTimer.Start(2.5f);
        }
    }
}

void World::Render(const Camera& camera, float aspectRatio) {

    if (gCharacterReady) {
        // Render all 5 characters (active in bright color, off-screen in muted color)
        for (size_t i = 0; i < characters.size(); ++i) {
            glm::vec3 characterColor;
            
            if (static_cast<int>(i) == activeCharacterIndex) {
                // Active character - bright color
                switch (characters[i]->GetType()) {
                    case CharacterType::Boyd:    characterColor = glm::vec3(1.0f, 0.0f, 0.0f); break;  // Red
                    case CharacterType::Jade:    characterColor = glm::vec3(0.0f, 1.0f, 1.0f); break;  // Cyan
                    case CharacterType::Tabitha: characterColor = glm::vec3(0.0f, 1.0f, 0.0f); break;  // Green
                    case CharacterType::Victor:  characterColor = glm::vec3(1.0f, 1.0f, 0.0f); break;  // Yellow
                    case CharacterType::Sara:    characterColor = glm::vec3(1.0f, 0.0f, 1.0f); break;  // Magenta
                }
            } else {
                // Off-screen character - muted color (60% opacity equivalent)
                characterColor = glm::vec3(0.65f);
                characterColor *= 0.6f;
            }
            
            RenderCharacterCube(gCharacterShader, gPlayerMesh, camera, aspectRatio, 
                              characters[i]->transform.position, glm::vec3(0.4f), characterColor);
        }
        
        // Render NPCs
        for (const NPC& npc : npcs) {
            RenderCharacterCube(gCharacterShader, gNpcMesh, camera, aspectRatio, npc.transform.position, glm::vec3(0.50f, 0.90f, 0.50f), npc.GetDebugColor());
        }

        // Render Enemies
        if (nightTime) {
            for (const Enemy& enemy : enemies) {
                RenderCharacterCube(gCharacterShader, gEnemyMesh, camera, aspectRatio, enemy.transform.position, glm::vec3(0.60f, 1.05f, 0.60f), enemy.GetDebugColor());
            }
        }

        const Quest* activeQuest = nullptr;
        if (questSystem && hasActiveQuest) {
            activeQuest = questSystem->GetCharacterQuest(activeQuestCharacter);
        }

        const int activeObjectiveIndex = activeQuest ? activeQuest->GetNextIncompleteObjectiveIndex() : -1;
        const std::string activeQuestCharacterName = activeQuest ? CharacterTypeToName(activeQuestCharacter) : "";

        const auto renderQuestBeacon = [&](const glm::vec3& basePosition, bool highlighted) {
            if (!gQuestMarkerReady) {
                return;
            }

            // Smaller floating marker with a soft pulse to draw attention without overwhelming
            const float seed = std::fmod(std::abs(basePosition.x * 31.0f + basePosition.z * 17.0f), 10.0f);
            const float pulse = 1.0f + 0.06f * std::sin(worldClock * 4.0f + seed);
            const glm::vec3 markerColor = highlighted ? glm::vec3(0.72f, 0.28f, 1.0f) : glm::vec3(0.35f, 0.12f, 0.12f);
            const glm::vec3 markerScale = highlighted ? glm::vec3(0.55f, 0.85f, 0.55f) * pulse : glm::vec3(0.38f, 0.50f, 0.38f) * pulse;
            RenderCharacterCube(gCharacterShader, gQuestMarkerMesh, camera, aspectRatio, basePosition + glm::vec3(0.0f, 1.25f, 0.0f), markerScale, markerColor);
        };

        for (const InteractionNode& node : interactionSystem.GetNodes()) {
            if (node.questObjectiveIndex < 0) {
                continue;
            }

            if (!node.requiredCharacter.empty() && node.requiredCharacter != activeQuestCharacterName) {
                continue;
            }

            const bool isNightOnlyQuest = !nightTime && (activeQuestCharacter == CharacterType::Jade || activeQuestCharacter == CharacterType::Tabitha);
            if (isNightOnlyQuest) {
                continue;
            }

            const bool isActiveStep = activeQuest && node.questObjectiveIndex == activeObjectiveIndex;
            const glm::vec3 offsetPosition = node.position;
            renderQuestBeacon(offsetPosition, isActiveStep && node.questObjectiveIndex >= 0);
        }

        // Render any runtime checkpoints (ids starting with "checkpoint_")
        for (const InteractionNode& node : interactionSystem.GetNodes()) {
            if (node.id.rfind("checkpoint_", 0) != 0) continue;
            const glm::vec3 offsetPosition = node.position;
            // Highlight color and scale for checkpoint flags (smaller but distinct)
            const glm::vec3 checkpointColor = glm::vec3(1.0f, 0.85f, 0.0f);
            const glm::vec3 checkpointScale = glm::vec3(1.0f, 1.3f, 0.8f);
            RenderCharacterCube(gCharacterShader, gQuestMarkerMesh, camera, aspectRatio, offsetPosition + glm::vec3(0.0f, 1.4f, 0.0f), checkpointScale, checkpointColor);
        }
    }
    
    // Debug: Render interior zone boundaries
    RenderInteriorZoneBounds(camera, aspectRatio);
}


void World::InitializePlaceholderWorldRules() {
    if (storyLocations.empty()) {
        // Spread story locations across the full map area
        storyLocations.push_back({"sheriff_station", glm::vec3(-9.0f, 0.0f, 8.0f), 3.0f});      // NW
        storyLocations.push_back({"diner", glm::vec3(-35.0f, 0.0f, -35.0f), 3.0f});             // South-west edge
        storyLocations.push_back({"church", glm::vec3(-8.5f, 0.0f, -7.0f), 3.0f});              // SW
        storyLocations.push_back({"tunnel_entrance", glm::vec3(0.0f, 0.0f, -9.5f), 3.0f});      // South center
        storyLocations.push_back({"colony_house", glm::vec3(9.0f, 0.0f, 8.0f), 3.0f});         // Colony house (moved into visible area)
        storyLocations.push_back({"victor_hideout", glm::vec3(-11.0f, 0.0f, -1.0f), 3.0f});     // West
    }

    if (shelterZones.empty()) {
        // Shelter zones co-located with story locations
        shelterZones.push_back({"sheriff_station_shelter", glm::vec3(-9.0f, 0.0f, 8.0f), 3.0f, true, true});
        shelterZones.push_back({"diner_shelter", glm::vec3(-35.0f, 0.0f, -35.0f), 3.0f, true, true});
        shelterZones.push_back({"colony_house_shelter", glm::vec3(9.0f, 0.0f, 8.0f), 3.0f, true, true});
        shelterZones.push_back({"church_shelter", glm::vec3(-8.5f, 0.0f, -7.0f), 3.0f, true, true});
        
        // House shelters
        shelterZones.push_back({"house_1_shelter", glm::vec3(-25.0f, 0.0f, 0.0f), 5.0f, true, true});
        shelterZones.push_back({"house_2_shelter", glm::vec3(25.0f, 0.0f, 0.0f), 5.0f, true, true});
        shelterZones.push_back({"house_3_shelter", glm::vec3(-25.0f, 0.0f, 20.0f), 5.0f, true, true});
        shelterZones.push_back({"house_4_shelter", glm::vec3(25.0f, 0.0f, 20.0f), 5.0f, true, true});
        shelterZones.push_back({"house_5_shelter", glm::vec3(-25.0f, 0.0f, -20.0f), 5.0f, true, true});
        shelterZones.push_back({"house_6_shelter", glm::vec3(25.0f, 0.0f, -20.0f), 5.0f, true, true});
        shelterZones.push_back({"house_7_shelter", glm::vec3(0.0f, 0.0f, 20.0f), 5.0f, true, true});
    }

    std::cout << "[World] Placeholder story locations and talisman shelter zones initialized.\n";
}

void World::EnsureNpcDialogueCooldowns() {
    npcTalkCooldowns.resize(npcs.size());
    npcPanicCooldowns.resize(npcs.size(), 0.0f);
    npcNeighborTalkCooldowns.resize(npcs.size(), 0.0f);
}

bool World::TryNpcDialogue(Character& character, bool explicitInteraction) {
    EnsureNpcDialogueCooldowns();

    if (activeCharacterIndex < 0 || activeCharacterIndex >= static_cast<int>(characters.size())) {
        return false;
    }

    std::size_t bestNpcIndex = npcs.size();
    float bestDistance = 9999.0f;
    for (std::size_t npcIndex = 0; npcIndex < npcs.size(); ++npcIndex) {
        const float distance = HorizontalDistance(character.transform.position, npcs[npcIndex].transform.position);
        if (distance < 2.75f && distance < bestDistance) {
            bestDistance = distance;
            bestNpcIndex = npcIndex;
        }
    }

    if (bestNpcIndex == npcs.size()) {
        return false;
    }

    const std::size_t characterIndex = static_cast<std::size_t>(activeCharacterIndex);
    if (!explicitInteraction && npcTalkCooldowns[bestNpcIndex][characterIndex] > 0.0f) {
        return false;
    }

    NPC& npc = npcs[bestNpcIndex];
    const std::string npcLine = GetNpcLine(npc, character, worldClock);
    const std::string characterReply = GetCharacterNpcReplyLine(npc, character, worldClock);
    std::ostringstream dialogue;
    dialogue << npc.GetName() << ": " << npcLine << " | " << character.GetName() << ": " << characterReply;
    lastNpcDialogue = dialogue.str();
    npcDialogueDisplayTimer.Start(kNpcDialogueDisplayDuration);

    if (explicitInteraction) {
        TriggerDialogueNarrativeHooks(character, npc, npcLine, characterReply);
    }

    std::cout << "[Talk] " << character.GetName() << " and " << lastNpcDialogue << "\n";
    npcTalkCooldowns[bestNpcIndex][characterIndex] = explicitInteraction ? 1.0f : 5.0f;
    return true;
}

bool World::HandleInteractionOutcome(Character& activeChar, bool didInteract, const char* noTargetLog) {
    if (didInteract) {
        eventBus.Publish(InteractionTriggeredEvent{activeChar.GetType(), "world_interaction"});

        if (interactionSystem.HasLastQuestObjective()) {
            // Prevent starting/continuing quests or puzzles at night if the active character is exposed
            if (nightTime && !IsProtectedByShelter(activeChar.transform.position)) {
                lastInteractionFeedback = "Quests and puzzles are paused until morning.";
                lastInteractionFeedbackTimer.Start(2.5f);
                return true;
            }

            CharacterType questChar = interactionSystem.GetLastInteractionQuestCharacter();
            
            // Check day/night puzzle restrictions
            if (nightTime && (questChar == CharacterType::Boyd || questChar == CharacterType::Victor)) {
                lastInteractionFeedback = "It's too dangerous to focus on this at night.";
                lastInteractionFeedbackTimer.Start(2.5f);
                return true;
            }
            if (!nightTime && (questChar == CharacterType::Tabitha || questChar == CharacterType::Jade)) {
                lastInteractionFeedback = "The clues only reveal themselves in the dark.";
                lastInteractionFeedbackTimer.Start(2.5f);
                return true;
            }

            SetActiveQuest(questChar);

            Quest* quest = questSystem->GetCharacterQuest(interactionSystem.GetLastInteractionQuestCharacter());
            const int objectiveIndex = interactionSystem.GetLastInteractionQuestObjectiveIndex();
            if (quest && objectiveIndex >= 0) {
                const auto& objectives = quest->GetObjectives();
                if (objectiveIndex < static_cast<int>(objectives.size())) {
                    const QuestObjective& objective = objectives[objectiveIndex];
                    const int subObjectiveIndex = interactionSystem.GetLastInteractionQuestSubObjectiveIndex();
                    if (objective.type != ObjectiveType::Collect || (objectiveIndex == 1 && subObjectiveIndex == 1)) {
                        if (puzzleManager.StartPuzzle(interactionSystem.GetLastInteractionQuestCharacter(), objectiveIndex, subObjectiveIndex, objective, quest->GetTitle())) {
                            lastInteractionFeedback = "Puzzle opened: " + objective.description;
                            lastInteractionFeedbackTimer.Start(2.5f);
                            std::cout << "[Quest] Puzzle opened for objective " << objectiveIndex << "\n";
                        }
                    } else {
                        bool allCollected = true;
                        const std::string requiredCharacter = CharacterTypeToName(interactionSystem.GetLastInteractionQuestCharacter());
                        bool foundCollectNode = false;
                        for (const InteractionNode& node : interactionSystem.GetNodes()) {
                            if (node.questObjectiveIndex != objectiveIndex || node.requiredCharacter != requiredCharacter) {
                                continue;
                            }
                            if (node.questFlag.empty()) {
                                continue;
                            }
                            foundCollectNode = true;
                            if (!questSystem->HasStoryFlag(node.questFlag)) {
                                allCollected = false;
                                break;
                            }
                        }

                        if (foundCollectNode && allCollected) {
                            questSystem->AdvanceObjective(interactionSystem.GetLastInteractionQuestCharacter(), objectiveIndex);
                            interactionSystem.MarkQuestStepSolved(interactionSystem.GetLastInteractionQuestCharacter(), objectiveIndex);
                            lastInteractionFeedback = "Evidence complete. Go to the Evidence Board and press E.";
                            lastInteractionFeedbackTimer.Start(3.5f);
                            std::cout << "[Quest] Collect objective complete via evidence flags for objective " << objectiveIndex << "\n";
                        }
                    }
                }
            }
        }
        return true;
    }

    if (TryNpcDialogue(activeChar, true)) {
        return true;
    }

    std::cout << noTargetLog << "\n";
    return false;
}

bool World::TryActiveCharacterInteraction() {
    if (!questSystem) {
        return false;
    }

    Character* activeChar = GetActiveCharacter();
    if (!activeChar) {
        return false;
    }

    if (TryCalmNearbyNpc(*activeChar)) {
        return true;
    }

    if (nightTime && activeChar->GetType() == CharacterType::Victor) {
        lastInteractionFeedback = "Victor needs to stay inside and sleep.";
        lastInteractionFeedbackTimer.Start(2.5f);
        return false;
    }

    const bool didInteract = interactionSystem.TryInteract(*activeChar, *questSystem, hasActiveQuest, activeQuestCharacter);
    return HandleInteractionOutcome(*activeChar, didInteract, "[Interaction] Nothing nearby to interact with.");
}

bool World::TryActiveCharacterAbility() {
    if (!questSystem) {
        return false;
    }

    Character* activeChar = GetActiveCharacter();
    if (!activeChar) {
        return false;
    }

    if (activeChar->GetType() == CharacterType::Boyd && nightTime) {
        if (TryCalmNearbyNpc(*activeChar)) {
            activeChar->StartAbilityCooldown(8.0f);
            return true;
        }

        return false;
    }

    activeChar->ActivateAbility();
    return true;
}

bool World::TryActiveCharacterPickup() {
    if (!questSystem) {
        return false;
    }

    Character* activeChar = GetActiveCharacter();
    if (!activeChar) {
        return false;
    }

    const bool didInteract = interactionSystem.TryPickup(*activeChar, *questSystem, hasActiveQuest, activeQuestCharacter);
    return HandleInteractionOutcome(*activeChar, didInteract, "[Interaction] Nothing nearby to pick up.");
}

bool World::TryCalmNearbyNpc(Character& activeChar) {
    if (!nightTime || activeChar.GetType() != CharacterType::Boyd) {
        return false;
    }

    for (NPC& npc : npcs) {
        if (npc.GetFear() > 50.0f || npc.GetAIState() == NPCAIState::Panic) {
            const float distance = HorizontalDistance(activeChar.transform.position, npc.transform.position);
            if (distance < 3.0f) {
                npc.SetThreatPosition(glm::vec3(0.0f), false);
                std::cout << "[Boyd] Calmed down " << npc.GetName() << ".\n";
                lastInteractionFeedback = "You calmed down " + npc.GetName() + ".";
                lastInteractionFeedbackTimer.Start(2.5f);
                return true;
            }
        }
    }

    return false;
}

std::string World::GetInteractionPrompt() const {
    if (puzzleManager.IsActive()) {
        return "Puzzle Active";
    }

    const Character* activeChar = GetActiveCharacter();

    if (!activeChar || !questSystem) {
        return "";
    }

    const std::string nodePrompt = interactionSystem.GetPromptFor(*activeChar, *questSystem, hasActiveQuest, activeQuestCharacter);
    if (!nodePrompt.empty()) {
        return nodePrompt;
    }

    const NPC* bestNpc = nullptr;
    float bestNpcDistance = 9999.0f;
    for (const NPC& npc : npcs) {
        const float distance = HorizontalDistance(activeChar->transform.position, npc.transform.position);
        if (distance < 2.75f && distance < bestNpcDistance) {
            bestNpcDistance = distance;
            bestNpc = &npc;
        }
    }

    if (bestNpc) {
        return "Talk " + bestNpc->GetName();
    }

    return interactionSystem.GetPromptFor(*activeChar, *questSystem, hasActiveQuest, activeQuestCharacter);
}

bool World::NearestInteractionIsPickup() const {
    const Character* activeChar = GetActiveCharacter();

    if (!activeChar || !questSystem) {
        return false;
    }

    const InteractionNode* bestNode = FindNearestInteractionNode(interactionSystem.GetNodes(), *activeChar, *questSystem);

    return bestNode && bestNode->type == InteractionType::ItemPickup;
}

bool World::HasStoryFlag(const std::string& flag) const {
    return questSystem ? questSystem->HasStoryFlag(flag) : false;
}

void World::ApplyPuzzleConsequence(const std::string& consequence) {
    if (!questSystem || consequence.empty()) {
        return;
    }

    if (consequence == "tabitha_whisper_hidden_path_open") {
        questSystem->SetStoryFlag("tabitha_whisper_hidden_path_open");
        tabithaWhisperTempRouteTimer = 24.0f;
        lastInteractionFeedback = "The tunnel shivers and a hidden route appears for a short time.";
        lastInteractionFeedbackTimer.Start(4.0f);
        std::cout << "[World] Tabitha hidden path opened temporarily.\n";
        EnsureTabithaWhisperRouteNodes();
        return;
    }

    if (consequence == "tabitha_whisper_child_memory_awakened") {
        questSystem->SetStoryFlag("tabitha_whisper_child_memory_awakened");
        lastInteractionFeedback = "A child drawing holds still long enough to be remembered.";
        lastInteractionFeedbackTimer.Start(4.0f);
        std::cout << "[World] Tabitha child-memory state unlocked permanently.\n";
        EnsureTabithaWhisperRouteNodes();
        return;
    }

    if (consequence == "tabitha_whisper_false_chamber_active") {
        questSystem->SetStoryFlag("tabitha_whisper_false_chamber_active");
        tabithaWhisperFalseChamberTimer = 16.0f;
        lastInteractionFeedback = "The tunnel offers a false safe zone, then breathes wrong.";
        lastInteractionFeedbackTimer.Start(4.0f);
        std::cout << "[World] Tabitha false chamber branch activated.\n";
        EnsureTabithaWhisperRouteNodes();
        return;
    }

    if (consequence == "tabitha_whisper_creature_attracted") {
        questSystem->SetStoryFlag("tabitha_whisper_creature_attracted");
        lastMonsterScream = "A creature stirs in the dark after the wrong whisper.";
        monsterScreamDisplayTimer.Start(kScreamDisplayDuration);
        lastInteractionFeedback = "The wrong interpretation draws something closer.";
        lastInteractionFeedbackTimer.Start(3.5f);
        std::cout << "[World] Tabitha wrong interpretation attracted danger.\n";
        return;
    }
}

void World::EnsureTabithaWhisperRouteNodes() {
    if (!questSystem) {
        return;
    }

    const auto& nodes = interactionSystem.GetNodes();
    const auto hasNode = [&nodes](const std::string& id) {
        return std::any_of(nodes.begin(), nodes.end(), [&id](const InteractionNode& node) {
            return node.id == id;
        });
    };

    if (questSystem->HasStoryFlag("tabitha_whisper_hidden_path_open") && !hasNode("tabitha_whisper_hidden_route_door")) {
        InteractionNode node;
        node.id = "tabitha_whisper_hidden_route_door";
        node.type = InteractionType::Door;
        node.name = "Hidden Whisper Route";
        node.position = glm::vec3(-9.2f, 0.0f, -6.8f);
        node.radius = 2.6f;
        node.active = true;
        node.open = false;
        node.prompt = "Open";
        node.successMessage = "A narrow seam in the stone yields to the children' whispered guidance.";
        node.questFlag = "tabitha_whisper_hidden_route_entered";
        node.requiredFlag = "tabitha_whisper_hidden_path_open";
        node.requiredCharacter = "Tabitha";
        interactionSystem.AddNode(node);
    }

    if (questSystem->HasStoryFlag("tabitha_whisper_child_memory_awakened") && !hasNode("tabitha_whisper_child_memory")) {
        InteractionNode node;
        node.id = "tabitha_whisper_child_memory";
        node.type = InteractionType::Trigger;
        node.name = "Child Memory Hollow";
        node.position = glm::vec3(-6.7f, 0.0f, -4.9f);
        node.radius = 2.8f;
        node.active = true;
        node.prompt = "Inspect";
        node.successMessage = "The drawings stop being random. Tabitha feels the children's fear and hope as one memory.";
        node.questFlag = "tabitha_whisper_child_memory_seen";
        node.requiredFlag = "tabitha_whisper_child_memory_awakened";
        node.requiredCharacter = "Tabitha";
        interactionSystem.AddNode(node);
    }

    if (questSystem->HasStoryFlag("tabitha_whisper_false_chamber_active") && !hasNode("tabitha_whisper_false_chamber")) {
        InteractionNode node;
        node.id = "tabitha_whisper_false_chamber";
        node.type = InteractionType::Trigger;
        node.name = "False Chamber";
        node.position = glm::vec3(-10.5f, 0.0f, -7.6f);
        node.radius = 2.5f;
        node.active = true;
        node.prompt = "Enter";
        node.successMessage = "The chamber feels safe until the whispers go silent. It was only a lure.";
        node.questFlag = "tabitha_whisper_false_chamber_seen";
        node.requiredFlag = "tabitha_whisper_false_chamber_active";
        node.requiredCharacter = "Tabitha";
        interactionSystem.AddNode(node);
    }
}

WorldSaveState World::CaptureSaveState() const {
    WorldSaveState state;
    state.worldClock = worldClock;
    state.activeCharacterIndex = activeCharacterIndex;
    state.playerKilled = playerKilled;
    state.questState = questSystem ? questSystem->SerializeState() : std::string();
    state.puzzleState = puzzleManager.SerializeState();

    for (std::size_t i = 0; i < characters.size() && i < state.characters.size(); ++i) {
        state.characters[i].position = characters[i]->transform.position;
        state.characters[i].health = characters[i]->GetHealth();
    }

    return state;
}

void World::RestoreSaveState(const WorldSaveState& state) {
    worldClock = state.worldClock;
    playerKilled = state.playerKilled;
    activeCharacterIndex = glm::clamp(state.activeCharacterIndex, 0, static_cast<int>(characters.size()) - 1);

    if (questSystem) {
        questSystem->DeserializeState(state.questState);
    }
    puzzleManager.DeserializeState(state.puzzleState);
    puzzleManager.CancelActivePuzzle();

    for (std::size_t i = 0; i < characters.size() && i < state.characters.size(); ++i) {
        characters[i]->transform.position = state.characters[i].position;
        characters[i]->SetHealth(state.characters[i].health);
    }

    hasPreviousActivePosition = false;
}

bool World::SaveToFile(const std::string& path) const {
    std::ofstream file(path);
    if (!file.is_open()) {
        std::cerr << "[Save] Could not write " << path << "\n";
        return false;
    }

    const WorldSaveState state = CaptureSaveState();
    file << "fromville_save_v2\n";
    file << state.worldClock << ' ' << state.activeCharacterIndex << ' ' << state.playerKilled << '\n';
    file << std::quoted(state.questState) << '\n';
    file << std::quoted(state.puzzleState) << '\n';
    for (const CharacterSimState& characterState : state.characters) {
        file << characterState.position.x << ' '
             << characterState.position.y << ' '
             << characterState.position.z << ' '
             << characterState.health << '\n';
    }

    std::cout << "[Save] Wrote " << path << "\n";
    return true;
}

bool World::LoadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[Save] Could not open " << path << "\n";
        return false;
    }

    std::string header;
    file >> header;
    if (header != "fromville_save_v1" && header != "fromville_save_v2") {
        std::cerr << "[Save] Unsupported save file: " << path << "\n";
        return false;
    }

    WorldSaveState state;
    file >> state.worldClock >> state.activeCharacterIndex >> state.playerKilled;
    if (header == "fromville_save_v2") {
        file >> std::quoted(state.questState);
        file >> std::quoted(state.puzzleState);
    }
    for (CharacterSimState& characterState : state.characters) {
        file >> characterState.position.x
             >> characterState.position.y
             >> characterState.position.z
             >> characterState.health;
    }

    if (!file.good() && !file.eof()) {
        std::cerr << "[Save] Failed while reading " << path << "\n";
        return false;
    }

    RestoreSaveState(state);
    std::cout << "[Save] Loaded " << path << "\n";
    return true;
}

void World::SetActiveQuest(CharacterType characterType) {
    const Quest* quest = questSystem ? questSystem->GetCharacterQuest(characterType) : nullptr;
    if (quest && quest->IsComplete()) {
        hasActiveQuest = false;
        lastInteractionFeedback = "That quest is already complete.";
        lastInteractionFeedbackTimer.Start(2.0f);
        return;
    }

    activeQuestCharacter = characterType;
    hasActiveQuest = true;
    std::cout << "[Quest] Active quest set for character type " << static_cast<int>(characterType) << "\n";
    // Provide explicit instruction when a quest is chosen
    if (quest) {
        lastInteractionFeedback = std::string("Quest started: ") + quest->GetTitle() + ". Follow the helper and collect glyphs with F to progress.";
        lastInteractionFeedbackTimer.Start(4.0f);
    }
}

std::string World::GetQuestHelperText() const {
    if (!questSystem) {
        return "";
    }

    if (puzzleManager.IsActive()) {
        const std::string clue = puzzleManager.GetOverlayClue();
        const std::string title = puzzleManager.GetOverlayTitle();
        if (!title.empty()) {
            return "INVESTIGATE: " + title + (clue.empty() ? "" : " | " + clue);
        }
    }

    // Determine which character's quest to show help for
    CharacterType questCharacter;
    
    // Use active quest character if one is set, otherwise use current active character
    if (hasActiveQuest) {
        questCharacter = activeQuestCharacter;
    } else if (!characters.empty()) {
        questCharacter = characters[activeCharacterIndex]->GetType();
    } else {
        return "";
    }

    const Quest* quest = questSystem->GetCharacterQuest(questCharacter);
    if (!quest || quest->IsComplete()) {
        return "";
    }

    if (!nightTime && (questCharacter == CharacterType::Jade || questCharacter == CharacterType::Tabitha)) {
        return "";
    }

    const int nextObjectiveIndex = quest->GetNextIncompleteObjectiveIndex();
    const auto& objectives = quest->GetObjectives();
    if (nextObjectiveIndex < 0 || nextObjectiveIndex >= static_cast<int>(objectives.size())) {
        return "";
    }

    const QuestObjective& currentObjective = objectives[nextObjectiveIndex];
    const std::string objective = currentObjective.description;

    const Character* playerChar = nullptr;
    if (activeCharacterIndex >= 0 && activeCharacterIndex < static_cast<int>(characters.size())) {
        playerChar = characters[activeCharacterIndex].get();
    }

    const InteractionNode* activeNode = FindQuestInteractionNode(
        interactionSystem.GetNodes(),
        playerChar,
        *questSystem,
        questCharacter,
        nextObjectiveIndex,
        currentObjective);

    if (activeNode) {
        const float distance = playerChar ? HorizontalDistance(playerChar->transform.position, activeNode->position) : 0.0f;
        const int meters = static_cast<int>(std::round(distance));
        const std::string distanceText = " — " + std::to_string(meters) + "m";
        if (activeNode->type == InteractionType::ItemPickup) {
            return "HELP: go to " + activeNode->name + " and press F to collect: " + objective + distanceText;
        }
        return "HELP: go to " + activeNode->name + " and press E to complete: " + objective + distanceText;
    }

    return "HELP: " + objective;
}

std::string World::GetQuestWaypointText() const {
    if (!questSystem) {
        return "";
    }

    // Determine which character's quest to show waypoint for
    CharacterType questCharacter;
    if (hasActiveQuest) {
        questCharacter = activeQuestCharacter;
    } else if (!characters.empty()) {
        questCharacter = characters[activeCharacterIndex]->GetType();
    } else {
        return "";
    }

    const Quest* quest = questSystem->GetCharacterQuest(questCharacter);
    if (!quest || quest->IsComplete()) {
        return "";
    }

    if (!nightTime && (questCharacter == CharacterType::Jade || questCharacter == CharacterType::Tabitha)) {
        return "";
    }

    const int nextObjectiveIndex = quest->GetNextIncompleteObjectiveIndex();
    const auto& objectives = quest->GetObjectives();
    if (nextObjectiveIndex < 0 || nextObjectiveIndex >= static_cast<int>(objectives.size())) {
        return "";
    }

    const QuestObjective& currentObjective = objectives[nextObjectiveIndex];

    const Character* playerChar = characters[activeCharacterIndex].get();
    const InteractionNode* activeNode = FindQuestInteractionNode(
        interactionSystem.GetNodes(),
        playerChar,
        *questSystem,
        questCharacter,
        nextObjectiveIndex,
        currentObjective);

    if (activeNode && !characters.empty()) {
        const Character* playerChar = characters[activeCharacterIndex].get();
        const glm::vec3 delta = activeNode->position - playerChar->transform.position;
        const float distance = HorizontalDistance(playerChar->transform.position, activeNode->position);
        
        // Determine cardinal direction
        std::string direction = "?";
        const float angle = std::atan2(delta.z, delta.x) * 180.0f / 3.14159f;
        if (angle > -45 && angle <= 45) {
            direction = "East";
        } else if (angle > 45 && angle <= 135) {
            direction = "South";
        } else if (angle > 135 || angle <= -135) {
            direction = "West";
        } else {
            direction = "North";
        }
        
        const int meters = static_cast<int>(std::round(distance));
        return "◄ " + direction + " ► " + activeNode->name + " [" + std::to_string(meters) + "m]";
    }

    return "";
}

void World::AbandonActiveQuest() {
    if (hasActiveQuest) {
        hasActiveQuest = false;
        puzzleManager.CancelActivePuzzle();
        lastInteractionFeedback = "Quest abandoned.";
        lastInteractionFeedbackTimer.Start(2.0f);
        std::cout << "[Quest] Quest abandoned.\n";
    }
}

bool World::IsPuzzleActive() const {
    return puzzleManager.IsActive();
}

void World::UpdatePuzzle(float dt, const InputContext& input) {
    puzzleManager.Update(dt, input);
    if (puzzleManager.ConsumeSpawnRestartRequest()) {
        RestartFromSpawn();
        spawnRestartRequested = true;
    }
}

void World::RenderPuzzleOverlay(TextRenderer& textRenderer, int screenWidth, int screenHeight) const {
    puzzleManager.Render(textRenderer, screenWidth, screenHeight);
}

void World::RenderNarrativeOverlays(TextRenderer& textRenderer, int screenWidth, int screenHeight) const {
    const AtmosphereManager& atmosphere = AtmosphereManager::Instance();
    const float tension = atmosphere.GetGlobalTension();
    const float corruption = atmosphere.GetCorruptionLevel();
    const float symbolProgress = SymbolSystem::Instance().GetDecodingProgress();
    const float tunnelProgress = TunnelMappingSystem::Instance().GetMappingProgress();
    const float trauma = MemoryReplaySystem::Instance().GetTraumaLevel();
    const float mercy = MoralCorruptionSystem::Instance().GetMercyScore();
    const float vengeance = MoralCorruptionSystem::Instance().GetVengeanceScore();

    std::ostringstream line1;
    line1 << "TENSION " << static_cast<int>(tension * 100.0f) << "%  CORRUPTION " << static_cast<int>(corruption * 100.0f) << "%";
    std::ostringstream line2;
    line2 << "DECODING " << static_cast<int>(symbolProgress * 100.0f) << "%  MAPPING " << static_cast<int>(tunnelProgress * 100.0f) << "%";
    std::ostringstream line3;
    line3 << "TRAUMA " << static_cast<int>(trauma * 100.0f) << "%  MERCY " << static_cast<int>(mercy * 100.0f) << "%  REVENGE " << static_cast<int>(vengeance * 100.0f) << "%";

    textRenderer.RenderText(line1.str(), 24.0f, 34.0f, 0.42f, glm::vec3(0.80f, 0.92f, 1.0f), screenWidth, screenHeight);
    textRenderer.RenderText(line2.str(), 24.0f, 56.0f, 0.42f, glm::vec3(0.86f, 0.78f, 1.0f), screenWidth, screenHeight);
    textRenderer.RenderText(line3.str(), 24.0f, 78.0f, 0.38f, glm::vec3(1.0f, 0.72f, 0.58f), screenWidth, screenHeight);

    AtmosphereManager::Instance().Render(textRenderer, screenWidth, screenHeight);
    MemoryReplaySystem::Instance().Render();
    SymbolSystem::Instance().Render();
    TunnelMappingSystem::Instance().Render();
}

void World::RestartFromSpawn() {
    if (hasInitialSpawnState) {
        RestoreSaveState(initialSpawnState);
    }
    hasActiveQuest = false;
    lastInteractionFeedback = "You wake back at spawn.";
    lastInteractionFeedbackTimer.Start(2.5f);
}

bool World::ConsumeSpawnRestartRequest() {
    const bool requested = spawnRestartRequested;
    spawnRestartRequested = false;
    return requested;
}

void World::UpdateTimeOfDay(float dt) {
    worldClock += dt;
    constexpr float dayNightCycle = 120.0f;
    constexpr float sunsetTime = 60.0f; // DayNightCycle starts at sunrise, so sunset is halfway through the loop.
    const float cyclePosition = std::fmod(worldClock, dayNightCycle);
    const bool wasNight = nightTime;
    nightTime = cyclePosition >= sunsetTime;
    if (nightTime != wasNight) {
        std::cout << (nightTime ? "[World] Sunset falls. Talisman shelters are active.\n"
                                : "[World] Morning breaks. Town routines resume.\n");
        // Notify NPCs and enemies of time change
        for (NPC& npc : npcs) {
            npc.SetNight(nightTime);
        }
        for (Enemy& enemy : enemies) {
            enemy.SetNight(nightTime);
        }
    }
}

void World::UpdateOffscreenCharacters(float dt) {
    for (std::size_t i = 0; i < characters.size(); ++i) {
        if (static_cast<int>(i) == activeCharacterIndex) {
            continue;
        }

        Character& character = *characters[i];
        const glm::vec3 goal = nightTime ? GetNearestShelterCenter(character.transform.position)
                                         : GetCharacterStoryGoal(character.GetType());
        const bool moving = MoveCharacterToward(character, goal, dt);
        (void)moving;
        character.Update(dt);
    }
}

bool World::IsProtectedByShelter(const glm::vec3& position) const {
    if (!nightTime) {
        return false;
    }

    for (const ShelterZone& shelter : shelterZones) {
        if (!shelter.talismanPlaced || !shelter.sealed) {
            continue;
        }

        if (HorizontalDistance(position, shelter.center) <= shelter.radius) {
            return true;
        }
    }

    return false;
}

glm::vec3 World::GetNearestShelterCenter(const glm::vec3& position) const {
    if (shelterZones.empty()) {
        return position;
    }

    const ShelterZone* bestShelter = &shelterZones.front();
    float bestDistance = HorizontalDistance(position, bestShelter->center);
    for (const ShelterZone& shelter : shelterZones) {
        const float distance = HorizontalDistance(position, shelter.center);
        if (distance < bestDistance) {
            bestDistance = distance;
            bestShelter = &shelter;
        }
    }

    return bestShelter->center;
}

glm::vec3 World::GetCharacterStoryGoal(CharacterType type) const {
    const auto findLocation = [this](const std::string& id, const glm::vec3& fallback) {
        for (const LocationZone& location : storyLocations) {
            if (location.id == id) {
                return location.center;
            }
        }
        return fallback;
    };

    switch (type) {
        case CharacterType::Boyd:
            return findLocation("sheriff_station", glm::vec3(0.0f, 0.0f, 8.0f));
        case CharacterType::Jade:
            return findLocation("church", glm::vec3(8.0f, 0.0f, -2.0f));
        case CharacterType::Tabitha:
            return findLocation("tunnel_entrance", glm::vec3(-8.0f, 0.0f, -6.0f));
        case CharacterType::Victor:
            return findLocation("victor_hideout", glm::vec3(-9.0f, 0.0f, 5.0f));
        case CharacterType::Sara:
            return findLocation("diner", glm::vec3(-6.0f, 0.0f, 1.0f));
    }

    return glm::vec3(0.0f);
}

bool World::MoveCharacterToward(Character& character, const glm::vec3& target, float dt) {
    glm::vec3 direction = target - character.transform.position;
    direction.y = 0.0f;
    if (glm::length(direction) <= 0.55f) {
        return false;
    }

    direction = glm::normalize(direction);
    character.Move(direction.x, direction.z, dt);
    character.transform.rotation.y = glm::degrees(std::atan2(direction.x, direction.z));
    return true;
}

void World::TryAdvanceOffscreenStory(Character& character, std::size_t index, float dt, bool atGoal) {
    (void)character;
    (void)index;
    (void)dt;
    (void)atGoal;
    // Quest progress should only come from the player's active interactions and puzzles.
    // Offscreen simulation may move characters around the flat test arena, but it must
    // not silently complete objectives for characters the player has not used yet.
    return;

    if (!questSystem || index >= offscreenStoryTimers.size()) {
        return;
    }

    if (!atGoal) {
        offscreenStoryTimers[index] = 0.0f;
        return;
    }

    offscreenStoryTimers[index] += dt;
    if (offscreenStoryTimers[index] < 28.0f) {
        return;
    }

    offscreenStoryTimers[index] = 0.0f;
    Quest* quest = questSystem->GetCharacterQuest(character.GetType());
    if (!quest || quest->IsComplete() || quest->IsFailed()) {
        return;
    }

    const std::vector<QuestObjective>& objectives = quest->GetObjectives();
    for (std::size_t objectiveIndex = 0; objectiveIndex < objectives.size(); ++objectiveIndex) {
        if (!objectives[objectiveIndex].completed) {
            questSystem->AdvanceObjective(character.GetType(), static_cast<int>(objectiveIndex));
            std::cout << "[Sim] " << character.GetName() << " advanced off-screen: "
                      << objectives[objectiveIndex].description << "\n";
            return;
        }
    }
}

void World::RenderInteriorZoneBounds(const Camera& camera, float aspectRatio) const {
    // Debug: Interior zones are defined in code. Adjust coordinates in World::Initialize() if needed.
    (void)camera;
    (void)aspectRatio;
}

// ============================================================
// Terrain project: building rendering with day/night lighting
// ============================================================
void World::RenderObjects(const Camera& camera, float aspectRatio, const DayNightCycle& dayNight, float fogDensity) {
    if (!gBuildingReady) return;

    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(60.0f), aspectRatio, 0.1f, 500.0f);

    glm::vec3 lightDir = dayNight.getActiveLightDir();
    glm::vec3 lightColor = dayNight.getLightColor();
    glm::vec3 ambient = dayNight.getAmbientColor();
    float diffuseStrength = dayNight.getDiffuseStrength();
    glm::vec3 fogColor = dayNight.getFogColor();
    glm::vec3 viewPos = camera.GetPosition();

    auto RenderBuilding = [&](WorldMesh& wm, glm::vec3 pos, float rot, float scale) {
        if (!wm.valid) return;
        float adjustedY = -wm.minY * scale + 0.05f;
        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(pos.x, adjustedY, pos.z));
        model = glm::rotate(model, glm::radians(rot), glm::vec3(0.0f, 1.0f, 0.0f));
        if (scale != 1.0f) model = glm::scale(model, glm::vec3(scale));

        gBuildingShader.Bind();
        gBuildingShader.SetMat4("model", model);
        gBuildingShader.SetMat4("view", view);
        gBuildingShader.SetMat4("projection", projection);
        gBuildingShader.SetVec3("lightDir", lightDir);
        gBuildingShader.SetVec3("lightColor", lightColor);
        gBuildingShader.SetVec3("ambient", ambient);
        gBuildingShader.SetFloat("diffuseStrength", diffuseStrength);
        gBuildingShader.SetVec3("viewPos", viewPos);
        gBuildingShader.SetVec3("fogColor", fogColor);
        gBuildingShader.SetFloat("fogDensity", fogDensity);
        wm.mesh.Draw();
    };

    // Render all loaded house instances so the town actually shows multiple houses
    RenderBuilding(gHouseMesh, glm::vec3(-25.0f, 0, 0.0f), 90.0f, 1.0f);
    RenderBuilding(gHouseMesh, glm::vec3(25.0f, 0, 0.0f), -90.0f, 1.0f);
    RenderBuilding(gHouseMesh, glm::vec3(-25.0f, 0, 20.0f), 90.0f, 1.0f);
    RenderBuilding(gHouseMesh, glm::vec3(25.0f, 0, 20.0f), -90.0f, 1.0f);
    RenderBuilding(gHouseMesh, glm::vec3(-25.0f, 0, -20.0f), 90.0f, 1.0f);
    RenderBuilding(gHouseMesh, glm::vec3(25.0f, 0, -20.0f), -90.0f, 1.0f);
    RenderBuilding(gHouseMesh, glm::vec3(0.0f, 0, 20.0f), 0.0f, 1.0f);
    RenderBuilding(gDinnerMesh, glm::vec3(-35.0f, 0, -35.0f), 0.0f, 1.0f);
    RenderBuilding(gHouseMesh, glm::vec3(9.0f, 0, 8.0f), 0.0f, 1.0f);
    RenderBuilding(gPoliceMesh, glm::vec3(-9.0f, 0, 8.0f), 180.0f, 2.0f);

    // Render doors
    for (auto& door : doors) {
        door.Render(gBuildingShader, view, projection, lightDir, lightColor, ambient, diffuseStrength, viewPos, fogColor, fogDensity);
    }
}

// ============================================================
// Terrain project: door enter/exit system
// ============================================================
void World::TryInteract() {
    Character* activeChar = GetActiveCharacter();
    if (!activeChar) return;

    glm::vec3 charPos = activeChar->transform.position;

    for (auto& door : doors) {
        float dist = glm::length(charPos - door.GetPosition());
        if (dist < 3.0f) {
            if (!isInsideBuilding) {
                previousOutsidePosition = charPos;
                activeChar->transform.position = door.GetInsidePosition();
                isInsideBuilding = true;
                door.Interact();
                std::cout << "[World] Entered building: " << door.GetName() << "\n";
            }
            return;
        }
    }

    // Also try the game logic interaction system
    TryActiveCharacterInteraction();
}

void World::TryExit() {
    if (!isInsideBuilding) return;
    Character* activeChar = GetActiveCharacter();
    if (!activeChar) return;
    activeChar->transform.position = previousOutsidePosition;
    isInsideBuilding = false;
    std::cout << "[World] Exited building.\n";
}
