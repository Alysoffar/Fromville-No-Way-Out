#include "game/world/World.h"

#include <cmath>
#include <cctype>
#include <iomanip>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <cfloat>
#include <iostream>
#include <vector>
#include <map>

#include <glm/gtc/matrix_transform.hpp>

#include "engine/renderer/Camera.h"
#include "engine/audio/AudioManager.h"
#include "engine/renderer/Mesh.h"
#include "engine/renderer/Shader.h"
#include "engine/renderer/TextRenderer.h"
#include "engine/core/InputManager.h"
#include "engine/renderer/TerrainRenderer.h"
#include "engine/resources/loader.h"
#include "game/atmosphere/AtmosphereManager.h"
#include "game/memory/MemoryReplaySystem.h"
#include "game/moral/MoralCorruptionSystem.h"
#include "game/symbols/SymbolSystem.h"
#include "game/tunnels/TunnelMappingSystem.h"
#include "game/runtime/GameplayEvents.h"
#include "game/world/map_manager.h"
#include "game/story/StoryManager.h"
#include "game/dialogue/DialogueManager.h"
#include "engine/resources/loader.h"
#include "game/world/DayNightCycle.h"
#include "game/entities/Door.h"
#include <GLFW/glfw3.h>

// =============================================================================
// Player Mesh
// =============================================================================

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

namespace {

struct WorldMesh {
    Mesh mesh;
    float minY = 0.0f;
    bool valid = false;
};

WorldMesh gPlayerMesh;
WorldMesh gTerrainMesh;
WorldMesh gHouseMesh;
WorldMesh gDinnerMesh;
WorldMesh gPoliceMesh;
Shader gPlayerShader("Player");
bool gPlayerReady = false;

Mesh gVillageMesh;
Shader gVillageShader("VillageModel");
bool gVillageReady = false;


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
constexpr float kArenaHalfExtent = 12.0f;

struct DialogueContext {
    bool night = false;
    float tension = 0.0f;
    float corruption = 0.0f;
    float fear = 0.0f;
    std::string location;
};

std::string ToLowerCopy(std::string value) {
    for (char& c : value) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return value;
}

bool ContainsToken(const std::string& text, const char* token) {
    return text.find(token) != std::string::npos;
}

std::string GetDialogueLocationLabel(const std::vector<LocationZone>& locations, const glm::vec3& position) {
    const LocationZone* bestLocation = nullptr;
    float bestDistance = std::numeric_limits<float>::max();

    for (const LocationZone& location : locations) {
        const glm::vec2 delta = glm::vec2(position.x - location.center.x, position.z - location.center.z);
        const float distance = glm::length(delta);
        if (distance <= location.radius + 1.5f && distance < bestDistance) {
            bestDistance = distance;
            bestLocation = &location;
        }
    }

    if (!bestLocation) {
        return "";
    }

    const std::string lowered = ToLowerCopy(bestLocation->id);
    if (ContainsToken(lowered, "sheriff")) return "sheriff_station";
    if (ContainsToken(lowered, "diner")) return "diner";
    if (ContainsToken(lowered, "church")) return "church";
    if (ContainsToken(lowered, "tunnel")) return "tunnel";
    if (ContainsToken(lowered, "colony")) return "colony_house";
    if (ContainsToken(lowered, "victor")) return "victor_hideout";
    return lowered;
}

DialogueContext BuildDialogueContext(const std::vector<LocationZone>& locations, const glm::vec3& position, bool night, float fear) {
    const AtmosphereManager& atmosphere = AtmosphereManager::Instance();
    DialogueContext context;
    context.night = night;
    context.tension = atmosphere.GetGlobalTension();
    context.corruption = atmosphere.GetCorruptionLevel();
    context.fear = fear;
    context.location = GetDialogueLocationLabel(locations, position);
    return context;
}

std::string FragmentLine(std::string line, float intensity, std::size_t seed) {
    if (line.empty() || intensity <= 0.35f) {
        return line;
    }

    if ((seed % 3) == 0) {
        const std::size_t cut = line.find_first_of(",;:");
        if (cut != std::string::npos && cut > 6) {
            line = line.substr(0, cut);
        }
        if (line.empty() || line.back() != '.') {
            line += "...";
        } else {
            line.back() = '.';
            line += ".";
        }
    }

    return line;
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

std::string GetNpcLine(const NPC& npc, const Character& character, float worldClock, bool nightTime, const std::vector<LocationZone>& locations) {
    const DialogueContext context = BuildDialogueContext(locations, character.transform.position, nightTime, npc.GetFear());

    static const std::array<std::string, 10> dayGeneral = {
        "Did you hear the radio again last night?",
        "Something moved near the trees.",
        "People keep pretending this place makes sense.",
        "You ever notice the town changes when nobody's looking?",
        "The children were singing again.",
        "I don't think those symbols were there yesterday.",
        "Nobody talks about the far road anymore.",
        "You should stop going near the tunnels alone.",
        "I found mud on the inside of the window frame.",
        "The silence here feels like someone listening."
    };

    static const std::array<std::string, 10> nightGeneral = {
        "Did you lock the windows?",
        "Don't answer voices outside.",
        "Something knocked on the wall.",
        "I heard a child laughing.",
        "That thing knew my name.",
        "They're different tonight.",
        "Don't look outside after midnight.",
        "I think the town wants us awake.",
        "The hallway sounded crowded a minute ago.",
        "Please, just keep the door shut."
    };

    static const std::array<std::string, 4> dinerDay = {
        "The coffee tastes burnt again.",
        "Everybody gets quieter in the diner.",
        "You can hear the road from here if you stop talking.",
        "No one wants to sit by the windows anymore."
    };
    static const std::array<std::string, 4> dinerNight = {
        "Keep the blinds closed.",
        "The diner stops feeling safe after dark.",
        "I heard something under the booth seats.",
        "If the lights flicker, don't look at the door."
    };
    static const std::array<std::string, 4> churchDay = {
        "The candles burned down overnight.",
        "People come here when they need to think, not when they need answers.",
        "The bell never rings for a good reason.",
        "Sometimes prayer is just another way of keeping busy."
    };
    static const std::array<std::string, 4> churchNight = {
        "Don't listen if the bell sounds wrong.",
        "The church feels different after dark.",
        "I think something waits under the floorboards.",
        "Some nights even the prayers sound afraid."
    };
    static const std::array<std::string, 4> tunnelDay = {
        "The air down there makes people hear things.",
        "Don't go below alone.",
        "We mark the walls so we can pretend it helps.",
        "Every tunnel looks older than it should."
    };
    static const std::array<std::string, 4> tunnelNight = {
        "If you hear water, keep moving.",
        "Don't answer anything that sounds trapped.",
        "The tunnels feel awake tonight.",
        "I saw footprints where nobody walked."
    };
    static const std::array<std::string, 4> sheriffDay = {
        "The radio cut out before dawn.",
        "We need to keep the reports straight.",
        "If someone panics, we all lose the room.",
        "The sheriff station feels quieter than it should."
    };
    static const std::array<std::string, 4> sheriffNight = {
        "Lock the station door again.",
        "Don't trust the radio after dark.",
        "I can hear the generator from all the way outside.",
        "If someone knocks twice, wait before opening."
    };
    static const std::array<std::string, 4> colonyDay = {
        "Too many people are sleeping with one eye open.",
        "The house creaks when nobody moves.",
        "You can tell who slept and who just pretended.",
        "Everyone is trying not to be the one who speaks first."
    };
    static const std::array<std::string, 4> colonyNight = {
        "Stay away from the windows.",
        "The house sounds crowded even when it isn't.",
        "People keep waking up to the same sound.",
        "Nobody should be awake this late."
    };
    static const std::array<std::string, 4> hideoutDay = {
        "Victor draws the same places over and over.",
        "This room feels like a thought someone forgot.",
        "The quiet here makes it easier to remember things you would rather not.",
        "Somebody hid here for a long time."
    };
    static const std::array<std::string, 4> hideoutNight = {
        "Victor said this place only looks empty.",
        "The walls keep their own memories.",
        "I don't like how the shadows sit in the corners.",
        "If this room starts humming, leave."
    };

    const std::size_t npcHash = std::hash<std::string>{}(npc.GetName());
    const std::size_t charHash = static_cast<std::size_t>(character.GetType());
    const std::size_t timeComponent = static_cast<std::size_t>(std::fmod(worldClock, nightTime ? 6.0f : 10.0f));
    const std::size_t seed = npcHash + charHash * 7 + timeComponent * 13;

    const auto pick = [seed](const auto& pool) {
        return pool[seed % pool.size()];
    };

    std::string line;
    if (context.location == "diner") {
        line = nightTime ? pick(dinerNight) : pick(dinerDay);
    } else if (context.location == "church") {
        line = nightTime ? pick(churchNight) : pick(churchDay);
    } else if (context.location == "tunnel") {
        line = nightTime ? pick(tunnelNight) : pick(tunnelDay);
    } else if (context.location == "sheriff_station") {
        line = nightTime ? pick(sheriffNight) : pick(sheriffDay);
    } else if (context.location == "colony_house") {
        line = nightTime ? pick(colonyNight) : pick(colonyDay);
    } else if (context.location == "victor_hideout") {
        line = nightTime ? pick(hideoutNight) : pick(hideoutDay);
    } else {
        line = nightTime ? pick(nightGeneral) : pick(dayGeneral);
    }

    if (context.night && (context.tension > 0.55f || context.corruption > 0.4f)) {
        line = FragmentLine(line, context.tension + context.corruption, seed);
    } else if (!context.night && context.tension > 0.65f) {
        line = FragmentLine(line, context.tension, seed + 1);
    }

    return line;
}

std::string GetCharacterNpcReplyLine(const NPC& npc, const Character& character, float worldClock, bool nightTime, const std::vector<LocationZone>& locations) {
    const DialogueContext context = BuildDialogueContext(locations, npc.transform.position, nightTime, character.GetHealth() < 50.0f ? 0.6f : 0.2f);

    static const std::array<std::string, 5> boydDay = {
        "Keep it quiet. Tell me only what matters.",
        "If something is wrong, say it plainly.",
        "We stay calm first. Then we move.",
        "I need the details, not the fear.",
        "Nobody goes off alone when the town starts acting strange."
    };
    static const std::array<std::string, 5> boydNight = {
        "Check the doors. Then check them again.",
        "Stay where I can see you.",
        "Do not answer from the dark.",
        "If it sounds wrong, treat it like danger.",
        "We get through the night one room at a time."
    };
    static const std::array<std::string, 5> jadeDay = {
        "That is not random. Give me a minute.",
        "If it repeats, it means something.",
        "The pattern is the part nobody wants to see.",
        "Do not touch it yet. I need to think.",
        "Every bad idea in this town thinks it's a clue."
    };
    static const std::array<std::string, 5> jadeNight = {
        "No. That is not a coincidence.",
        "Listen. The timing matters.",
        "I can feel the pattern getting closer.",
        "If the lights change, tell me immediately.",
        "I am trying to stay rational. Help me do that."
    };
    static const std::array<std::string, 5> tabithaDay = {
        "If the house is changing, it will leave a trace.",
        "I have seen places like this hide things in plain sight.",
        "Mark it. Come back when the light is better.",
        "Do not let the children wander near the openings.",
        "The town always seems smaller after you notice the walls."
    };
    static const std::array<std::string, 5> tabithaNight = {
        "Stay near the light.",
        "If the walls shift, do not chase them.",
        "I can hear the house settling, and I do not like it.",
        "Keep your voice low. Something might answer.",
        "We are not going to solve this by panicking."
    };
    static const std::array<std::string, 5> victorDay = {
        "I remember this place. Not all of it, but enough.",
        "The trees do that when people are afraid.",
        "Don't ask the music to stay.",
        "I can draw it if you want.",
        "Some things are better when they stay where they are."
    };
    static const std::array<std::string, 5> victorNight = {
        "It feels closer at night.",
        "I know that sound. I just don't know from where.",
        "Please don't make me remember too fast.",
        "The dark is full of old things here.",
        "If I go quiet, wait for me."
    };
    static const std::array<std::string, 5> saraDay = {
        "I heard it too. I just don't know what to do with it.",
        "Sometimes I think the town is listening through us.",
        "I am trying not to say the wrong thing.",
        "If I seem off, it is because I am thinking too hard.",
        "Please don't ask me to explain everything at once."
    };
    static const std::array<std::string, 5> saraNight = {
        "I can hear it in the walls again.",
        "Don't let me be alone with my thoughts tonight.",
        "If I go quiet, that does not mean I am fine.",
        "The room feels wrong when it gets this dark.",
        "I think something is trying to sound like us."
    };

    const std::size_t npcHash = std::hash<std::string>{}(npc.GetName());
    const std::size_t index = (npcHash + static_cast<std::size_t>(worldClock)) % boydDay.size();
    const auto pick = [index](const auto& pool) { return pool[index % pool.size()]; };

    std::string line;
    switch (character.GetType()) {
        case CharacterType::Boyd: line = nightTime ? pick(boydNight) : pick(boydDay); break;
        case CharacterType::Jade: line = nightTime ? pick(jadeNight) : pick(jadeDay); break;
        case CharacterType::Tabitha: line = nightTime ? pick(tabithaNight) : pick(tabithaDay); break;
        case CharacterType::Victor: line = nightTime ? pick(victorNight) : pick(victorDay); break;
        case CharacterType::Sara: line = nightTime ? pick(saraNight) : pick(saraDay); break;
    }

    if (context.location == "church" && character.GetType() == CharacterType::Tabitha) {
        line = nightTime ? "The church does not feel like shelter tonight." : "Even the church feels like it is holding its breath.";
    } else if (context.location == "tunnel" && character.GetType() == CharacterType::Victor) {
        line = nightTime ? "I do not want to go back under there." : "I remember the tunnels better than I should.";
    } else if (context.location == "sheriff_station" && character.GetType() == CharacterType::Boyd) {
        line = nightTime ? "We keep the station quiet and the doors locked." : "We keep the station calm. That is the job.";
    }

    if (context.night && context.corruption > 0.45f) {
        line = FragmentLine(line, context.corruption, index + static_cast<std::size_t>(character.GetType()));
    }

    return line;
}

std::string GetCharacterPairLine(const Character& activeCharacter, const Character& otherCharacter, float worldClock, bool nightTime, const std::vector<LocationZone>& locations) {
    const DialogueContext context = BuildDialogueContext(locations, activeCharacter.transform.position, nightTime, 0.25f);

    static const std::array<std::string, 8> dayLines = {
        "Did you notice the way everyone stopped talking when the radio came on?",
        "Let's not make this a bigger problem than it already is.",
        "You heard it too. Good. I thought I was losing the thread.",
        "We should compare notes before somebody changes the subject.",
        "The town feels different when nobody says the obvious part out loud.",
        "If you saw something, tell me while it is still fresh.",
        "I keep thinking the answer is in the thing nobody wants to mention.",
        "Let's stay with what we know for now."
    };
    static const std::array<std::string, 8> nightLines = {
        "Keep your voice down.",
        "Do you hear that, or is it just me?",
        "We should stop talking and listen.",
        "If the room goes quiet, do not fill it.",
        "I do not like how close the walls sound tonight.",
        "If you need to leave, say it now.",
        "Nothing outside should know our names.",
        "Stay here until the noise passes."
    };

    const std::size_t index = (static_cast<std::size_t>(activeCharacter.GetType()) * 5 +
                               static_cast<std::size_t>(otherCharacter.GetType()) * 3 +
                               static_cast<std::size_t>(worldClock)) % dayLines.size();
    std::string line = nightTime ? nightLines[index % nightLines.size()] : dayLines[index % dayLines.size()];

    if (context.location == "tunnel") {
        line = nightTime ? "Not here. Not with the walls listening." : "Let's talk once we are back upstairs.";
    } else if (context.location == "church") {
        line = nightTime ? "Lower your voice. Even this place feels awake." : "There is nothing comforting about how quiet it is in here.";
    }

    if (nightTime && (context.tension > 0.5f || context.corruption > 0.35f)) {
        line = FragmentLine(line, context.tension + context.corruption, index + 2);
    }

    return activeCharacter.GetName() + " to " + otherCharacter.GetName() + ": " + line;
}

std::string GetNpcNeighborLine(const NPC& speaker, const NPC& listener, float worldClock, bool nightTime, const std::vector<LocationZone>& locations) {
    const DialogueContext context = BuildDialogueContext(locations, speaker.transform.position, nightTime, speaker.GetFear());

    static const std::array<std::string, 8> dayLines = {
        "Did you get any sleep?",
        "Keep your voice down until we know who is listening.",
        "The diner still has a little coffee left.",
        "Have you noticed how people avoid the road after dark?",
        "I am trying not to think too hard about last night.",
        "If you see Boyd, tell him the radio is acting strange again.",
        "I don't think everyone is telling the same story.",
        "We should stop pretending this is normal."
    };
    static const std::array<std::string, 8> nightLines = {
        "Are you awake?",
        "Don't say that name too loudly.",
        "I heard knocking from the empty room again.",
        "If the child laughs, don't answer.",
        "Something moved by the trees a second ago.",
        "We stay inside until the light comes back.",
        "I think the house knows we are scared.",
        "Please tell me you locked the back door."
    };

    const std::size_t speakerHash = std::hash<std::string>{}(speaker.GetName());
    const std::size_t listenerHash = std::hash<std::string>{}(listener.GetName());
    const std::size_t index = (speakerHash + listenerHash * 3 + static_cast<std::size_t>(worldClock)) % dayLines.size();
    std::string line = nightTime ? nightLines[index % nightLines.size()] : dayLines[index % dayLines.size()];

    if (context.location == "diner") {
        line = nightTime ? "The diner feels wrong this late." : "The diner is quiet enough to hear your own thoughts.";
    } else if (context.location == "colony_house") {
        line = nightTime ? "Nobody should be talking this loudly in the house." : "Everyone keeps trying to act normal in here.";
    }

    return speaker.GetName() + " to " + listener.GetName() + ": " + line;
}

std::string GetPanicLine(const Character& character) {
    static const std::array<std::string, 12> lines = {
        "Wait.",
        "Don't.",
        "Stay back.",
        "No, no...",
        "Please, don't leave me here.",
        "Don't look at it.",
        "Inside. Now.",
        "It knows.",
        "Get to the door.",
        "Keep moving.",
        "Too close.",
        "Shut it."
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
        "We can hear your heart.",
        "Keep hiding. It only changes the sound.",
        "That door was never enough.",
        "You look tired.",
        "We know where the others are.",
        "The town told us your name.",
        "You do not have to answer that.",
        "You cannot keep everyone safe.",
        "The dark is already inside.",
        "We remember you.",
        "Open the window.",
        "Someone is missing.",
        "Your friends are calling from the woods.",
        "We are closer than you think."
    };

    const std::size_t index = (static_cast<std::size_t>(character.GetType()) * 7 +
                               static_cast<std::size_t>(distance * 2.0f) +
                               static_cast<std::size_t>(worldClock)) % taunts.size();
    return taunts[index];
}

std::string GetCharacterMonsterResponseLine(const Character& character, float worldClock) {
    static const std::array<std::string, 5> lines = {
        "Boyd: Hold the line. Nobody moves.",
        "Jade: I am not helping you make this worse.",
        "Tabitha: I know what you want. It is not happening.",
        "Victor: I remember enough to know this is bad.",
        "Sara: No. I am not listening anymore."
    };

    const std::size_t index = static_cast<std::size_t>(character.GetType()) % lines.size();
    (void)worldClock;
    return lines[index];
}

std::string GetMonsterScreamLine(int enemyCount, float distance) {
    static const std::array<std::string, 12> screams = {
        "Something is moving too close.",
        "The dark outside is not empty.",
        "Listen. It is still there.",
        "Do not open the door.",
        "That sound is inside the house.",
        "Something is scratching at the wall.",
        "The hallway just changed.",
        "It is standing right there.",
        "Do not answer the knock.",
        "The room went cold.",
        "You are not alone in here.",
        "Someone else is breathing."
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
    // ---- Player mesh ----
    if (!gPlayerReady) {
        std::vector<MeshVertex> vertices;
        std::vector<unsigned int> indices;
        if (Loader::LoadOBJ("assets/models/boyd.obj", vertices, indices)) {
            gPlayerMesh.mesh.Create(vertices, indices);
            gPlayerReady = gPlayerMesh.mesh.IsValid();
        } else {
            std::cerr << "Failed to load character OBJ: assets/models/boyd.obj\n";
            CreateColoredCubeMesh(gPlayerMesh.mesh, glm::vec3(0.85f, 0.85f, 0.95f));
            gPlayerReady = gPlayerMesh.mesh.IsValid();
        }
    }
    
    // We'll load the Terrain mesh in LoadMeshes to reuse vertices for collision.
}

float CalculateMinY(const std::vector<MeshVertex>& vertices) {
    float minY = FLT_MAX;
    for (const auto& v : vertices) {
        minY = std::min(minY, v.position.y);
    }
    return minY;
}

void LoadMeshes(CollisionWorld* cw, std::vector<Door>& doors) {
    if (gCharacterReady) return;

    // ---- Terrain ground mesh ----
    std::cout << "[World] Attempting to load ground mesh: assets/models/Terrain003_1K.obj\n";
    if (!gTerrainMesh.valid) {
        std::vector<MeshVertex> vertices;
        std::vector<unsigned int> indices;
        if (Loader::LoadOBJ("assets/models/Terrain003_1K.obj", vertices, indices)) {
            gTerrainMesh.mesh.Create(vertices, indices);
            gTerrainMesh.minY = CalculateMinY(vertices);
            gTerrainMesh.valid = true;
            
            if (cw) {
                // Normalize terrain so its minimum point is at Y=0
                glm::mat4 terrainTransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -gTerrainMesh.minY, 0.0f));
                cw->AddTrianglesFromMesh(vertices, indices, terrainTransform);
            }
            std::cout << "[World] Ground mesh and collision loaded: Terrain003_1K.obj (" << vertices.size() << " vertices)\n";
        } else {
            std::cerr << "[World] CRITICAL ERROR: Failed to load ground mesh: assets/models/Terrain003_1K.obj\n";
        }
    }

    std::cout << "[World] Loading meshes...\n";

    std::vector<MeshVertex> vertices;
    std::vector<unsigned int> indices;

    if (Loader::LoadOBJ("assets/models/boyd.obj", vertices, indices)) {
        gPlayerMesh.mesh.Create(vertices, indices);
        gPlayerMesh.minY = CalculateMinY(vertices);
        gPlayerMesh.valid = gPlayerMesh.mesh.IsValid();
        std::cout << "  [OK] Player mesh loaded (minY=" << gPlayerMesh.minY << ")\n";
    } else {
        std::cerr << "  [FAIL] Could not load player mesh.\n";
        gPlayerMesh.valid = false;
    }

    auto LoadBuilding = [&](const std::string& path, WorldMesh& outMesh, glm::vec3 pos, float rot, float scale, const std::string& doorKeyword, float doorOpenAngle) {
        std::vector<OBJShape> shapes;
        if (!Loader::LoadOBJWithShapes(path, shapes)) return;

        std::vector<MeshVertex> mainVertices;
        std::vector<unsigned int> mainIndices;

        // First pass: Find global minY to correctly position everything, and compute local bounds
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

        // Compute true world center of the building geometry
        glm::vec3 localCenter = (localMin + localMax) * 0.5f;
        glm::mat4 localToWorld = glm::translate(glm::mat4(1.0f), buildingPos);
        localToWorld = glm::rotate(localToWorld, glm::radians(rot), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::vec3 buildingWorldCenter = glm::vec3(localToWorld * glm::vec4(localCenter, 1.0f));
        buildingWorldCenter.y = buildingPos.y + 0.5f; // place slightly above floor

        std::map<std::string, std::pair<std::vector<MeshVertex>, std::vector<unsigned int>>> doorGroups;

        for (auto& s : shapes) {
            if (s.name.find(doorKeyword) != std::string::npos &&
                s.name.find("Frame") == std::string::npos &&
                s.name.find("Sill") == std::string::npos) {
                
                std::string groupName = s.name;
                if (s.name.find("Knob") != std::string::npos) {
                    groupName = "Door_Main_Mesh"; // Merge knob with main door
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
            
            // If it's a right door, hinge is on the right (max X). Otherwise left (min X).
            float hingeX = (dName.find("_R") != std::string::npos) ? maxP.x : minP.x;
            glm::vec3 hingePos = glm::vec3(hingeX, 0.0f, minP.z);
            
            // Fix double doors so they swing outward correctly (reverse angle for right doors)
            float angle = doorOpenAngle;
            if (dName.find("_R") != std::string::npos) {
                angle = -doorOpenAngle;
            }
            
            // Calculate door center in world space
            glm::mat4 localToWorld = glm::translate(glm::mat4(1.0f), buildingPos);
            localToWorld = glm::rotate(localToWorld, glm::radians(rot), glm::vec3(0.0f, 1.0f, 0.0f));
            glm::vec3 doorWorldCenter = glm::vec3(localToWorld * glm::vec4(hingePos, 1.0f));
            
            Door d;
            // Store buildingPos as the position for rendering transform
            d.Initialize(dName, dData.first, dData.second, buildingPos, rot, angle, hingePos, doorWorldCenter);
            
            // We teleport the player exactly to the building's physical center
            d.SetInsidePosition(buildingWorldCenter);
            doors.push_back(std::move(d));
        }

        // If no doors were extracted (like Diner or Police), create a virtual door at the entrance
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

            glm::mat4 localToWorld = glm::translate(glm::mat4(1.0f), buildingPos);
            localToWorld = glm::rotate(localToWorld, glm::radians(rot), glm::vec3(0.0f, 1.0f, 0.0f));
            glm::vec3 entranceWorldPos = glm::vec3(localToWorld * glm::vec4(entranceLocalPos, 1.0f));

            Door virtualDoor;
            virtualDoor.Initialize(path + "_Entrance", {}, {}, buildingPos, rot, 0.0f, glm::vec3(0.0f), entranceWorldPos);
            
            // Teleport point inside: center of building
            glm::vec3 insidePos = buildingPos;
            insidePos.y += 0.5f; // safely above floor
            virtualDoor.SetInsidePosition(insidePos);
            
            doors.push_back(std::move(virtualDoor));
        }
        
        outMesh.mesh.Create(mainVertices, mainIndices);
        outMesh.minY = CalculateMinY(mainVertices);
        outMesh.valid = outMesh.mesh.IsValid();

        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(pos.x, adjustedY, pos.z));
        model = glm::rotate(model, glm::radians(rot), glm::vec3(0.0f, 1.0f, 0.0f));
        // We already scaled the vertices, so we don't scale the model matrix anymore!
        // Disable floor filtering for buildings to ensure interior navigation is solid.
        if (cw) cw->AddTrianglesFromMesh(mainVertices, mainIndices, model, false);
    };

    // House 1 & 2
    LoadBuilding("assets/models/House.obj", gHouseMesh, glm::vec3(-25.0f, 0, 0.0f), 90.0f, 1.0f, "Door_Main", 90.0f);
    LoadBuilding("assets/models/House.obj", gHouseMesh, glm::vec3(25.0f, 0, 0.0f), -90.0f, 1.0f, "Door_Main", 90.0f);

    LoadBuilding("assets/models/Dinner.obj", gDinnerMesh, glm::vec3(0.0f, 0, -40.0f), 0.0f, 2.0f, "DoorGlass", -90.0f);
    LoadBuilding("assets/models/Police.obj", gPoliceMesh, glm::vec3(0.0f, 0, 40.0f), 180.0f, 2.0f, "Door", 90.0f);

    gPlayerShader.Load("assets/shaders/model_lit.vert", "assets/shaders/model_lit.frag");
    gCharacterShader.Load("assets/shaders/model_lit.vert", "assets/shaders/model_lit.frag");
    gPlayerReady = true;
    gCharacterReady = true;
}


} // anonymous namespace

// =============================================================================
// World class implementation
// =============================================================================
World::World() {
    mapManager = std::make_unique<MapManager>();
    terrain = std::make_unique<TerrainRenderer>();
}

World::~World() = default;

void World::Initialize() {
    // terrain->Initialize();
    InitializeModels();

    std::vector<Door> doors; 
    LoadMeshes(&collisionWorld, doors);
    
    // Build collision BVH after meshes are loaded so we can raycast against them for grounding
    collisionWorld.BuildBVH();

    InitializeCharacters();
    GroundEntities();
    InitializePlaceholderWorldRules();

    entityManager.BindStorage(&characters, &npcs, &enemies, &activeCharacterIndex);
    entityManager.BindCollisionWorld(&collisionWorld);

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
            glfwPollEvents(); // Keep window responsive during long loads
        }

        InitializeVoiceCueLibrary(*audioManager);
        audioManager->PlaySound("ambient_tension_low", 0.30f);
    }

    // Provide DialogueManager with audio manager for voice/feedback
    DialogueManager::Instance().SetAudioManager(audioManager.get());

    // Subscribe dialogue reactions to gameplay events
    eventBus.Subscribe<PuzzleCompletedEvent>([](const PuzzleCompletedEvent& ev) {
        const std::string name = CharacterTypeToName(ev.character);
        if (!name.empty()) {
            DialogueManager::Instance().ModifyTrust(name, +5);
            DialogueManager::Instance().AddMemoryFlag(name, std::string("objective:") + std::to_string(ev.objectiveIndex));
        }
    });

    eventBus.Subscribe<InteractionTriggeredEvent>([](const InteractionTriggeredEvent& ev) {
        // small recognition boost when interacting with a node
        const std::string name = CharacterTypeToName(ev.character);
        if (!name.empty()) {
            DialogueManager::Instance().ModifyTrust(name, +1);
        }
    });

    eventBus.Subscribe<PromiseMadeEvent>([](const PromiseMadeEvent& ev) {
        const std::string name = CharacterTypeToName(ev.character);
        if (!name.empty()) {
            DialogueManager::Instance().MarkPromise(name, ev.promiseId);
            DialogueManager::Instance().ModifyTrust(name, +3);
        }
    });

    eventBus.Subscribe<PromiseBrokenEvent>([](const PromiseBrokenEvent& ev) {
        const std::string name = CharacterTypeToName(ev.character);
        if (!name.empty()) {
            DialogueManager::Instance().BreakPromise(name, ev.promiseId);
        }
    });

    eventBus.Subscribe<AccusationEvent>([](const AccusationEvent& ev) {
        const std::string name = CharacterTypeToName(ev.character);
        if (!name.empty()) {
            DialogueManager::Instance().MarkAccusation(name, ev.accusationId);
        }
    });

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
    if (!mapManager || !terrain) {
        return;
    }

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
        if (nightTime) {
            const glm::vec3 threatPosition = FindNearestEnemyPosition(enemies, npc.transform.position, kNpcSightRange, enemyVisible);
            npc.SetThreatPosition(threatPosition, enemyVisible);
            npcThreatened[i] = enemyVisible || npc.IsInDanger();
            if (npcThreatened[i] && !hasRescueTarget) {
                rescueTarget = npc.transform.position;
                hasRescueTarget = true;
            }
        } else {
            npc.SetThreatPosition(npc.transform.position, false);
            npc.SetRescueTarget(npc.transform.position, false);
            npcThreatened[i] = false;
        }
    }

    for (std::size_t i = 0; i < npcs.size(); ++i) {
        NPC& npc = npcs[i];
        const bool canRescue = nightTime && hasRescueTarget && !npcThreatened[i] && HorizontalDistance(npc.transform.position, rescueTarget) <= 12.0f;
        npc.SetRescueTarget(rescueTarget, canRescue);
        npc.Update(dt);
    }

    if (nightTime && !playerKilled && activeChar) {
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
                bestPerception.hasTarget = false;
                bestPerception.visible = false;
                bestPerception.sound = 0.0f;
                bestPerception.light = 0.0f;
                bestPerception.proximity = 0.0f;
            }

            float bestScore = ScorePerception(bestPerception);

            for (std::size_t i = 0; i < characters.size(); ++i) {
                if (static_cast<int>(i) == activeCharacterIndex) {
                    continue;
                }

                const Character& offscreenCharacter = *characters[i];
                if (IsProtectedByShelter(offscreenCharacter.transform.position)) {
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
                        if (nightTime) {
                            // Trigger monster scream only at night.
                            const float distance = HorizontalDistance(character.transform.position, enemy.transform.position);
                            lastMonsterScream = GetMonsterScreamLine(static_cast<int>(enemies.size()), distance);
                            monsterScreamDisplayTimer.Start(kScreamDisplayDuration);
                            PlayVoiceLine(audioManager.get(), "monster", lastMonsterScream, 0.95f);
                        } else {
                            lastMonsterScream.clear();
                            monsterScreamDisplayTimer.Stop();
                        }
                    }
                    
                    std::cout << "[Damage] " << character.GetName() << " takes " << damageAmount << " damage! HP: " << character.GetHealth() << "\n";
                    if (static_cast<int>(charIdx) == activeCharacterIndex) {
                        if (nightTime && !lastMonsterScream.empty()) {
                            std::cout << lastMonsterScream << "\n";
                        }
                    }

                    if (character.GetHealth() <= 0.0f && charIdx < initialSpawnState.characters.size()) {
                        std::cout << "[Respawn] " << character.GetName() << " died and respawned.\n";
                        character.transform.position = initialSpawnState.characters[charIdx].position;
                        character.SetHealth(100.0f);
                        if (static_cast<int>(charIdx) == activeCharacterIndex) {
                            lastDamageAmount = 0.0f;
                            lastDamageDisplayTimer.Stop();
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
                const std::string npcLine = GetNpcLine(npc, *activeChar, worldClock, nightTime, storyLocations);
                const std::string characterReply = GetCharacterNpcReplyLine(npc, *activeChar, worldClock, nightTime, storyLocations);
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
                lastNpcDialogue = GetCharacterPairLine(*activeChar, otherCharacter, worldClock, nightTime, storyLocations);
                npcDialogueDisplayTimer.Start(kNpcDialogueDisplayDuration);
                std::cout << "[Conversation] " << lastNpcDialogue << "\n";
                characterReactionCooldowns[characterIndex] = 6.0f;
            }
        }

        for (const Enemy& enemy : enemies) {
            const float distance = HorizontalDistance(activeChar->transform.position, enemy.transform.position);
            if (distance < 8.0f && characterMonsterInteractionCooldowns[activeIndex] <= 0.0f) {
                if (nightTime) {
                    lastMonsterScream = GetMonsterTauntLine(*activeChar, distance, worldClock);
                    monsterScreamDisplayTimer.Start(kScreamDisplayDuration);
                    lastNpcDialogue = GetCharacterMonsterResponseLine(*activeChar, worldClock);
                    npcDialogueDisplayTimer.Start(kNpcDialogueDisplayDuration);
                    std::cout << lastMonsterScream << "\n";
                    std::cout << "[Monster Response] " << lastNpcDialogue << "\n";
                    PlayVoiceLine(audioManager.get(), "monster", lastMonsterScream, 0.95f);
                }
                else {
                    lastMonsterScream.clear();
                    monsterScreamDisplayTimer.Stop();
                    lastNpcDialogue = GetCharacterMonsterResponseLine(*activeChar, worldClock);
                    npcDialogueDisplayTimer.Start(kNpcDialogueDisplayDuration);
                    std::cout << "[Day Response] " << lastNpcDialogue << "\n";
                }
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

    for (std::size_t npcIndex = 0; npcIndex < npcs.size(); ++npcIndex) {
        if (npcNeighborTalkCooldowns[npcIndex] > 0.0f) {
            continue;
        }

        for (std::size_t otherNpcIndex = npcIndex + 1; otherNpcIndex < npcs.size(); ++otherNpcIndex) {
            if (HorizontalDistance(npcs[npcIndex].transform.position, npcs[otherNpcIndex].transform.position) < 3.25f) {
                lastNpcDialogue = GetNpcNeighborLine(npcs[npcIndex], npcs[otherNpcIndex], worldClock, nightTime, storyLocations);
                npcDialogueDisplayTimer.Start(kNpcDialogueDisplayDuration);
                npcNeighborTalkCooldowns[npcIndex] = 9.0f;
                npcNeighborTalkCooldowns[otherNpcIndex] = 9.0f;
                std::cout << "[NPC Chat] " << lastNpcDialogue << "\n";
                const std::string speaker = GetSpeakerName(lastNpcDialogue);
                const std::string neighborVoiceGroup = IsFemaleSpeaker(speaker) ? "npc_female" : "npc_male";
                PlayVoiceLine(audioManager.get(), neighborVoiceGroup, lastNpcDialogue, 0.80f);
                break;
            }
        }
    }

    mapManager->Update(camera.GetPosition());
    // terrain->Update(*mapManager);
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

void World::Render(const Camera& camera, float aspectRatio, const DayNightCycle& dayNightCycle) {
    const glm::mat4 projection = camera.GetProjectionMatrix(aspectRatio);
    const glm::mat4 view = camera.GetViewMatrix();
    const glm::vec3 cameraPos = camera.GetPosition();

    // Lighting parameters
    glm::vec3 sunDir = dayNightCycle.getActiveLightDir();
    glm::vec3 lightColor = dayNightCycle.getLightColor();
    glm::vec3 ambientColor = dayNightCycle.getAmbientColor();
    float diffuseStrength = dayNightCycle.getDiffuseStrength();
    glm::vec3 fogColor = dayNightCycle.getFogColor();
    float fogDensity = 0.005f; // Reduced density to see further across the large terrain

    auto SetupLighting = [&](Shader& s) {
        s.Bind();
        s.SetVec3("uLightDir", sunDir);
        s.SetVec3("uLightColor", lightColor);
        s.SetVec3("uAmbient", ambientColor);
        s.SetFloat("uDiffuseStrength", diffuseStrength);
        s.SetVec3("uViewPos", cameraPos);
        s.SetVec3("uFogColor", fogColor);
        s.SetFloat("uFogDensity", fogDensity);
    };

    // 1. Ground (Rendered by TerrainRenderer in Game.cpp)
    // 2. Render Buildings
    auto DrawBuilding = [&](const WorldMesh& mesh, const glm::vec3& pos, float rot, float scale) {
        if (!mesh.valid) return;
        glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
        model = glm::rotate(model, glm::radians(rot), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(scale));
        
        SetupLighting(gPlayerShader);
        gPlayerShader.SetMat4("projection", projection);
        gPlayerShader.SetMat4("view", view);
        gPlayerShader.SetMat4("model", model);
        mesh.mesh.Draw();
        gPlayerShader.Unbind();
    };

    DrawBuilding(gHouseMesh, glm::vec3(-25.0f, 0, 0.0f), 90.0f, 1.0f);
    DrawBuilding(gHouseMesh, glm::vec3(25.0f, 0, 0.0f), -90.0f, 1.0f);
    DrawBuilding(gDinnerMesh, glm::vec3(0.0f, 0, -40.0f), 0.0f, 2.0f);
    DrawBuilding(gPoliceMesh, glm::vec3(0.0f, 0, 40.0f), 180.0f, 2.0f);

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
            
            RenderCharacterCube(gCharacterShader, gPlayerMesh.mesh, camera, aspectRatio, 
                              characters[i]->transform.position, glm::vec3(0.4f), characterColor);
        }
        
        // Render NPCs
        for (const NPC& npc : npcs) {
            RenderCharacterCube(gCharacterShader, gNpcMesh, camera, aspectRatio, npc.transform.position, glm::vec3(0.25f, 0.45f, 0.25f), npc.GetDebugColor());
        }

        // Render Enemies only at night
        if (nightTime) {
            for (const Enemy& enemy : enemies) {
                RenderCharacterCube(gCharacterShader, gEnemyMesh, camera, aspectRatio, enemy.transform.position, glm::vec3(0.30f, 0.55f, 0.30f), enemy.GetDebugColor());
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

        const auto renderProp = [&](const glm::vec3& position, const glm::vec3& scale, const glm::vec3& color) {
            RenderCharacterCube(gCharacterShader, gNpcMesh, camera, aspectRatio, position, scale, color);
        };

        // (Test props removed to clear the spawn area)

        // Special: render Bloody Knife with dark metallic base + red glow to make it obvious
        for (const InteractionNode& node : interactionSystem.GetNodes()) {
            if (node.id == "boyd_evidence_knife") {
                const glm::vec3 pos = node.position + glm::vec3(0.0f, 0.05f, 0.0f);
                // base 'knife' prop
                RenderCharacterCube(gCharacterShader, gNpcMesh, camera, aspectRatio, pos, glm::vec3(0.2f, 0.05f, 0.6f), glm::vec3(0.12f, 0.12f, 0.12f));
                // blood overlay (small red quad)
                const float glow = 0.4f + 0.6f * std::abs(std::sin(worldClock * 3.0f));
                RenderCharacterCube(gCharacterShader, gNpcMesh, camera, aspectRatio, pos + glm::vec3(0.0f, 0.12f, 0.0f), glm::vec3(0.22f, 0.02f, 0.62f), glm::vec3(0.6f * glow, 0.05f, 0.05f));
            }
        }

        const auto renderMarker = [&](const glm::vec3& position, const glm::vec3& color) {
            renderProp(position + glm::vec3(0.0f, 0.08f, 0.0f), glm::vec3(0.42f, 0.10f, 0.42f), color);
        };

        renderMarker(glm::vec3(0.0f, 0.0f, 6.0f), glm::vec3(1.0f, 0.18f, 0.12f));
        renderMarker(glm::vec3(4.5f, 0.0f, 0.0f), glm::vec3(1.0f, 0.18f, 0.12f));
        renderMarker(glm::vec3(6.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.18f, 0.12f));
        renderMarker(glm::vec3(6.0f, 0.0f, 6.0f), glm::vec3(1.0f, 0.18f, 0.12f));

        renderMarker(glm::vec3(0.0f, 0.0f, 6.0f), glm::vec3(0.0f, 0.95f, 1.0f));
        renderMarker(glm::vec3(-6.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.95f, 1.0f));
        renderMarker(glm::vec3(2.0f, 0.0f, 6.0f), glm::vec3(0.0f, 0.95f, 1.0f));

        renderMarker(glm::vec3(-6.0f, 0.0f, -6.0f), glm::vec3(0.0f, 0.9f, 0.25f));
        renderMarker(glm::vec3(-2.0f, 0.0f, -6.0f), glm::vec3(0.0f, 0.9f, 0.25f));

        renderMarker(glm::vec3(-4.0f, 0.0f, -2.0f), glm::vec3(1.0f, 0.92f, 0.1f));
        renderMarker(glm::vec3(0.0f, 0.0f, -4.0f), glm::vec3(1.0f, 0.92f, 0.1f));
        renderMarker(glm::vec3(2.0f, 0.0f, -6.0f), glm::vec3(1.0f, 0.92f, 0.1f));

        renderMarker(glm::vec3(1.7f, 0.0f, 3.0f), glm::vec3(1.0f, 0.0f, 1.0f));
        renderMarker(glm::vec3(6.0f, 0.0f, -2.0f), glm::vec3(1.0f, 0.0f, 1.0f));
        renderMarker(glm::vec3(4.0f, 0.0f, -6.0f), glm::vec3(1.0f, 0.0f, 1.0f));
    }
    
    // Debug: Render interior zone boundaries
    RenderInteriorZoneBounds(camera, aspectRatio);
}


void World::InitializePlaceholderWorldRules() {
    if (storyLocations.empty()) {
        // Spread story locations across the full map area
        storyLocations.push_back({"sheriff_station", glm::vec3(-9.0f, 0.0f, 8.0f), 3.0f});      // NW
        storyLocations.push_back({"diner", glm::vec3(9.0f, 0.0f, 7.5f), 3.0f});                 // NE
        storyLocations.push_back({"church", glm::vec3(-8.5f, 0.0f, -7.0f), 3.0f});              // SW
        storyLocations.push_back({"tunnel_entrance", glm::vec3(0.0f, 0.0f, -9.5f), 3.0f});      // South center
        storyLocations.push_back({"colony_house", glm::vec3(9.5f, 0.0f, -8.5f), 3.0f});         // SE
        storyLocations.push_back({"victor_hideout", glm::vec3(-11.0f, 0.0f, -1.0f), 3.0f});     // West
    }

    if (shelterZones.empty()) {
        // Shelter zones co-located with story locations
        shelterZones.push_back({"sheriff_station_shelter", glm::vec3(-9.0f, 0.0f, 8.0f), 3.0f, true, true});
        shelterZones.push_back({"diner_shelter", glm::vec3(9.0f, 0.0f, 7.5f), 3.0f, true, true});
        shelterZones.push_back({"colony_house_shelter", glm::vec3(9.5f, 0.0f, -8.5f), 3.0f, true, true});
        shelterZones.push_back({"church_shelter", glm::vec3(-8.5f, 0.0f, -7.0f), 3.0f, true, true});
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
    const std::string npcLine = GetNpcLine(npc, character, worldClock, nightTime, storyLocations);
    const std::string characterReply = GetCharacterNpcReplyLine(npc, character, worldClock, nightTime, storyLocations);
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
            const CharacterType questCharacter = interactionSystem.GetLastInteractionQuestCharacter();
            SetActiveQuest(questCharacter);

            if (questSystem && !questSystem->IsProgressAllowed(questCharacter, cyclePhase)) {
                lastInteractionFeedback = questSystem->GetProgressLockMessage(questCharacter, cyclePhase);
                if (lastInteractionFeedback.empty()) {
                    lastInteractionFeedback = "That progression window is closed right now.";
                }
                lastInteractionFeedbackTimer.Start(3.5f);
                std::cout << "[Quest] Progress blocked by cycle phase.\n";
                return true;
            }

            Quest* quest = questSystem->GetCharacterQuest(questCharacter);
            const int objectiveIndex = interactionSystem.GetLastInteractionQuestObjectiveIndex();
            if (quest && objectiveIndex >= 0) {
                const auto& objectives = quest->GetObjectives();
                if (objectiveIndex < static_cast<int>(objectives.size())) {
                    const QuestObjective& objective = objectives[objectiveIndex];
                    const int subObjectiveIndex = interactionSystem.GetLastInteractionQuestSubObjectiveIndex();
                    if (objective.type != ObjectiveType::Collect || (objectiveIndex == 1 && subObjectiveIndex == 1)) {
                        if (puzzleManager.StartPuzzle(questCharacter, objectiveIndex, subObjectiveIndex, objective, quest->GetTitle())) {
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
                            questSystem->AdvanceObjective(questCharacter, objectiveIndex);
                            interactionSystem.MarkQuestStepSolved(questCharacter, objectiveIndex);
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

    const bool didInteract = interactionSystem.TryInteract(*activeChar, *questSystem, hasActiveQuest, activeQuestCharacter);
    return HandleInteractionOutcome(*activeChar, didInteract, "[Interaction] Nothing nearby to interact with.");
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
    state.dialogueRelationships = DialogueManager::Instance().SerializeState();

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
    file << std::quoted(state.dialogueRelationships) << '\n';
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
        file >> std::quoted(state.dialogueRelationships);
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
    // restore dialogue relationships
    DialogueManager::Instance().DeserializeState(state.dialogueRelationships);
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
        const std::string progressWindow = questSystem ? questSystem->GetProgressWindowLabel(characterType) : std::string("DAY");
        lastInteractionFeedback = std::string("Quest started: ") + quest->GetTitle() + ". Progress window: " + progressWindow + ".";
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

    const DayCyclePhase phase = questSystem->GetCurrentDayCyclePhase();
    if (!questSystem->IsProgressAllowed(questCharacter, phase)) {
        std::string lockMessage = questSystem->GetProgressLockMessage(questCharacter, phase);
        if (lockMessage.empty()) {
            lockMessage = std::string("WAIT UNTIL ") + questSystem->GetProgressWindowLabel(questCharacter);
        }
        return lockMessage;
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

    const DayCyclePhase phase = questSystem->GetCurrentDayCyclePhase();
    if (!questSystem->IsProgressAllowed(questCharacter, phase)) {
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
    const DayCyclePhase previousPhase = cyclePhase;
    cyclePhase = QuestSystem::GetDayCyclePhaseFromClock(worldClock);
    nightTime = cyclePhase == DayCyclePhase::Night;
    if (cyclePhase != previousPhase) {
        switch (cyclePhase) {
            case DayCyclePhase::Morning:
                std::cout << "[World] Morning breaks. Town routines resume.\n";
                break;
            case DayCyclePhase::Afternoon:
                std::cout << "[World] Afternoon settles over the town.\n";
                break;
            case DayCyclePhase::Sunset:
                std::cout << "[World] Sunset begins. Everyone heads inside.\n";
                break;
            case DayCyclePhase::Night:
                std::cout << "[World] Night has fallen. Talisman shelters are active.\n";
                break;
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
    // TODO: Add wireframe box rendering for zones in the future
    (void)camera;  // Suppress unused parameter warning
    (void)aspectRatio;
}
