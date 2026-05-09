#include "game/world/World.h"

#include <cmath>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>

#include "engine/renderer/Camera.h"
#include "engine/renderer/Mesh.h"
#include "engine/renderer/Shader.h"
#include "engine/renderer/terrain.h"
#include "engine/resources/loader.h"
#include "game/world/map_manager.h"

namespace {
Mesh gPlayerMesh;
Shader gPlayerShader("PlayerModel");
bool gPlayerReady = false;

Mesh gVillageMesh;
Shader gVillageShader("VillageModel");
bool gVillageReady = false;


Mesh gNpcMesh;
Mesh gEnemyMesh;
Shader gCharacterShader("CharacterCubes");
bool gCharacterReady = false;

constexpr float kNpcSightRange = 9.0f;
constexpr float kEnemySightRange = 18.0f;
constexpr float kEnemyHearingRange = 26.0f;
constexpr float kEnemyLightRange = 22.0f;
constexpr float kArenaHalfExtent = 12.0f;

float HorizontalDistance(const glm::vec3& a, const glm::vec3& b) {
    glm::vec3 delta = a - b;
    delta.y = 0.0f;
    return glm::length(delta);
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
    gVillageReady = false;

    if (!gCharacterReady) {
        CreateColoredCubeMesh(gNpcMesh, glm::vec3(0.25f, 0.90f, 0.65f));
        CreateColoredCubeMesh(gEnemyMesh, glm::vec3(0.95f, 0.22f, 0.18f));
        gCharacterShader.Load("assets/shaders/model_lit.vert", "assets/shaders/model_lit.frag");
        gCharacterReady = gNpcMesh.IsValid() && gEnemyMesh.IsValid();
    }
}
}

World::World() = default;

void World::Initialize() {
    static MapManager worldMap;
    static TerrainRenderer terrainRenderer;

    mapManager = &worldMap;
    terrain = &terrainRenderer;

    terrain->Initialize();
    InitializeModels();
    InitializeCharacters();
    InitializePlaceholderWorldRules();

    // Create and initialize quest system with pointer to worldClock
    questSystem = std::make_unique<QuestSystem>(&worldClock);
    interactionSystem.Initialize();
    
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

    // Use a flat invisible collision plane for the temporary test arena.
    if (collisionWorld.LoadFlatGround(256.0f, 0.0f)) {
        std::cout << "[World] Flat test ground loaded successfully!\n";
    } else {
        std::cerr << "[World] Warning: Failed to load flat test ground, entities will use fallback physics.\n";
    }

    // Wire collision system to all characters
    for (auto& character : characters) {
        character->SetCollisionWorld(&collisionWorld);
    }

    for (NPC& npc : npcs) {
        npc.SetCollisionWorld(&collisionWorld);
    }

    for (Enemy& enemy : enemies) {
        enemy.SetCollisionWorld(&collisionWorld);
    }
    
    // Activate the first character (Boyd)
    if (!characters.empty()) {
        characters[activeCharacterIndex]->OnSwitchedTo();
    }
}


void World::Update(const Camera& camera, float dt) {
    if (!mapManager || !terrain) {
        return;
    }

    static std::array<float, 5> characterReactionCooldowns = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
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
    
    // Update character damage cooldowns
    for (float& cooldown : characterDamageCooldowns) {
        cooldown = std::max(0.0f, cooldown - dt);
    }
    for (float& cooldown : characterMonsterInteractionCooldowns) {
        cooldown = std::max(0.0f, cooldown - dt);
    }

    UpdateTimeOfDay(dt);

    // Update quest system (tracks story progression and consequences)
    if (questSystem) {
        questSystem->Update(dt);
        
        // Print any pending story consequences to console
        while (questSystem->HasPendingConsequences()) {
            const std::string& consequence = questSystem->GetNextConsequence();
            std::cout << "[Story Consequence] " << consequence << "\n";
        }

        interactionSystem.Update(dt, *questSystem);
    }

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
    static int frameCounter = 0;
    frameCounter++;
    if (frameCounter >= 120 && activeChar) {
        const auto& pos = activeChar->transform.position;
        std::cout << "[" << activeChar->GetName() << "] Pos: x=" << pos.x << " y=" << pos.y << " z=" << pos.z 
                  << " | Health: " << activeChar->GetHealth() << "\n";
        frameCounter = 0;
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
                        lastDamageTime = kDamageDisplayDuration;
                        
                        // Trigger monster scream
                        const float distance = HorizontalDistance(character.transform.position, enemy.transform.position);
                        lastMonsterScream = GetMonsterScreamLine(static_cast<int>(enemies.size()), distance);
                        monsterScreamDisplayTime = kScreamDisplayDuration;
                    }
                    
                    std::cout << "[Damage] " << character.GetName() << " takes " << damageAmount << " damage! HP: " << character.GetHealth() << "\n";
                    if (static_cast<int>(charIdx) == activeCharacterIndex) {
                        std::cout << lastMonsterScream << "\n";
                    }
                }
            }
        }
    }
    
    // Update monster scream and damage display timers
    monsterScreamDisplayTime = std::max(0.0f, monsterScreamDisplayTime - dt);
    lastDamageTime = std::max(0.0f, lastDamageTime - dt);
    npcDialogueDisplayTime = std::max(0.0f, npcDialogueDisplayTime - dt);

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
                npcDialogueDisplayTime = kNpcDialogueDisplayDuration;
                
                std::cout << "[Talk] " << activeChar->GetName() << " and " << lastNpcDialogue << "\n";
                npcTalkCooldowns[npcIndex][activeIndex] = 5.0f;
            }

            if (npc.IsInDanger() && npcPanicCooldowns[npcIndex] <= 0.0f) {
                const std::string panicLine = GetPanicLine(*activeChar);
                std::ostringstream panic;
                panic << npc.GetName() << ": " << panicLine;
                lastNpcDialogue = panic.str();
                npcDialogueDisplayTime = kNpcDialogueDisplayDuration;
                
                std::cout << "[Panic] " << lastNpcDialogue << "\n";
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
                npcDialogueDisplayTime = kNpcDialogueDisplayDuration;
                std::cout << "[Conversation] " << lastNpcDialogue << "\n";
                characterReactionCooldowns[characterIndex] = 6.0f;
            }
        }

        for (const Enemy& enemy : enemies) {
            const float distance = HorizontalDistance(activeChar->transform.position, enemy.transform.position);
            if (distance < 8.0f && characterMonsterInteractionCooldowns[activeIndex] <= 0.0f) {
                lastMonsterScream = GetMonsterTauntLine(*activeChar, distance, worldClock);
                monsterScreamDisplayTime = kScreamDisplayDuration;
                lastNpcDialogue = GetCharacterMonsterResponseLine(*activeChar, worldClock);
                npcDialogueDisplayTime = kNpcDialogueDisplayDuration;
                std::cout << lastMonsterScream << "\n";
                std::cout << "[Monster Response] " << lastNpcDialogue << "\n";
                characterMonsterInteractionCooldowns[activeIndex] = 7.5f;
                break;
            }

            if (distance < 6.5f && characterReactionCooldowns[activeCharacterIndex] <= 0.0f) {
                lastNpcDialogue = activeChar->GetName() + ": " + GetPanicLine(*activeChar);
                npcDialogueDisplayTime = kNpcDialogueDisplayDuration;
                std::cout << "[Panic] " << lastNpcDialogue << "\n";
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
                lastNpcDialogue = GetNpcNeighborLine(npcs[npcIndex], npcs[otherNpcIndex], worldClock);
                npcDialogueDisplayTime = kNpcDialogueDisplayDuration;
                npcNeighborTalkCooldowns[npcIndex] = 9.0f;
                npcNeighborTalkCooldowns[otherNpcIndex] = 9.0f;
                std::cout << "[NPC Chat] " << lastNpcDialogue << "\n";
                break;
            }
        }
    }

    mapManager->Update(camera.GetPosition());
    terrain->Update(*mapManager);
}

void World::Render(const Camera& camera, float aspectRatio) {
    if (!terrain) {
        return;
    }

    terrain->Render(camera, aspectRatio);

    const glm::mat4 projection = camera.GetProjectionMatrix(aspectRatio);
    const glm::mat4 view = camera.GetViewMatrix();

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
            RenderCharacterCube(gCharacterShader, gNpcMesh, camera, aspectRatio, npc.transform.position, glm::vec3(0.25f, 0.45f, 0.25f), npc.GetDebugColor());
        }

        // Render Enemies
        for (const Enemy& enemy : enemies) {
            RenderCharacterCube(gCharacterShader, gEnemyMesh, camera, aspectRatio, enemy.transform.position, glm::vec3(0.30f, 0.55f, 0.30f), enemy.GetDebugColor());
        }
    }
    
    // Debug: Render interior zone boundaries
    RenderInteriorZoneBounds(camera, aspectRatio);
}

void World::InitializeCharacters() {
    if (!characters.empty()) {
        return;
    }

    // Create all 5 playable characters at different spawn points
    characters.emplace_back(std::make_unique<Boyd>(glm::vec3(0.0f, 0.1f, 1.5f)));
    characters.emplace_back(std::make_unique<Jade>(glm::vec3(-1.8f, 0.1f, 1.5f)));
    characters.emplace_back(std::make_unique<Tabitha>(glm::vec3(1.8f, 0.1f, 1.5f)));
    characters.emplace_back(std::make_unique<Victor>(glm::vec3(-2.4f, 0.1f, -0.6f)));
    characters.emplace_back(std::make_unique<Sara>(glm::vec3(2.4f, 0.1f, -0.6f)));

    // Create NPCs
    if (npcs.empty()) {
        npcs.emplace_back("Mara", glm::vec3(-1.6f, 0.0f, 3.2f));
        npcs.emplace_back("Elena", glm::vec3(1.7f, 0.0f, 3.0f));
        npcs.emplace_back("Tom", glm::vec3(3.4f, 0.0f, -0.8f));
    }
    EnsureNpcDialogueCooldowns();

    // Create enemies
    if (enemies.empty()) {
        enemies.emplace_back(glm::vec3(5.5f, 0.0f, 5.5f));
        enemies.emplace_back(glm::vec3(-5.5f, 0.0f, -4.5f));
    }
}

void World::InitializePlaceholderWorldRules() {
    if (storyLocations.empty()) {
        storyLocations.push_back({"sheriff_station", glm::vec3(0.0f, 0.0f, 3.5f), 4.0f});
        storyLocations.push_back({"diner", glm::vec3(3.5f, 0.0f, 3.5f), 4.0f});
        storyLocations.push_back({"church", glm::vec3(-3.5f, 0.0f, 3.5f), 4.0f});
        storyLocations.push_back({"tunnel_entrance", glm::vec3(-4.5f, 0.0f, 0.0f), 4.0f});
        storyLocations.push_back({"colony_house", glm::vec3(4.5f, 0.0f, 0.0f), 4.0f});
        storyLocations.push_back({"victor_hideout", glm::vec3(0.0f, 0.0f, -4.0f), 4.0f});
    }

    if (shelterZones.empty()) {
        shelterZones.push_back({"sheriff_station_shelter", glm::vec3(0.0f, 0.0f, 3.5f), 4.0f, true, true});
        shelterZones.push_back({"diner_shelter", glm::vec3(3.5f, 0.0f, 3.5f), 4.0f, true, true});
        shelterZones.push_back({"colony_house_shelter", glm::vec3(4.5f, 0.0f, 0.0f), 4.0f, true, true});
        shelterZones.push_back({"church_shelter", glm::vec3(-3.5f, 0.0f, 3.5f), 4.0f, true, true});
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
    npcDialogueDisplayTime = kNpcDialogueDisplayDuration;

    std::cout << "[Talk] " << character.GetName() << " and " << lastNpcDialogue << "\n";
    npcTalkCooldowns[bestNpcIndex][characterIndex] = explicitInteraction ? 1.0f : 5.0f;
    return true;
}

bool World::TryActiveCharacterInteraction() {
    if (!questSystem) {
        return false;
    }

    Character* activeChar = GetActiveCharacter();
    if (!activeChar) {
        return false;
    }

    if (TryNpcDialogue(*activeChar, true)) {
        return true;
    }

    const bool didInteract = interactionSystem.TryInteract(*activeChar, *questSystem);
    if (!didInteract) {
        std::cout << "[Interaction] Nothing nearby to interact with.\n";
    }
    return didInteract;
}

std::string World::GetInteractionPrompt() const {
    const Character* activeChar = nullptr;
    if (activeCharacterIndex >= 0 && activeCharacterIndex < static_cast<int>(characters.size())) {
        activeChar = characters[activeCharacterIndex].get();
    }

    if (!activeChar || !questSystem) {
        return "";
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

    return interactionSystem.GetPromptFor(*activeChar, *questSystem);
}

bool World::HasStoryFlag(const std::string& flag) const {
    return questSystem ? questSystem->HasStoryFlag(flag) : false;
}

WorldSaveState World::CaptureSaveState() const {
    WorldSaveState state;
    state.worldClock = worldClock;
    state.activeCharacterIndex = activeCharacterIndex;
    state.playerKilled = playerKilled;

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
    file << "fromville_save_v1\n";
    file << state.worldClock << ' ' << state.activeCharacterIndex << ' ' << state.playerKilled << '\n';
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
    if (header != "fromville_save_v1") {
        std::cerr << "[Save] Unsupported save file: " << path << "\n";
        return false;
    }

    WorldSaveState state;
    file >> state.worldClock >> state.activeCharacterIndex >> state.playerKilled;
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

void World::SwitchCharacter(int index) {
    if (index < 0 || index >= static_cast<int>(characters.size())) {
        std::cerr << "[World] Invalid character index: " << index << "\n";
        return;
    }

    if (index == activeCharacterIndex) {
        return;  // Already active
    }

    // Call OnSwitchedFrom on previous character
    characters[activeCharacterIndex]->OnSwitchedFrom();
    std::cout << "[World] Switched from " << characters[activeCharacterIndex]->GetName() << "\n";

    // Switch to new character
    activeCharacterIndex = index;
    characters[activeCharacterIndex]->OnSwitchedTo();
    std::cout << "[World] Switched to " << characters[activeCharacterIndex]->GetName() << "\n";

    hasPreviousActivePosition = false;
    previousActivePosition = characters[activeCharacterIndex]->transform.position;
    
    // TODO: Move camera to new character position
    // TODO: Update HUD for new character stats
    // TODO: Apply post-processing effects (sepia for Victor memory mode, etc)
}

void World::UpdateTimeOfDay(float dt) {
    worldClock += dt;
    constexpr float dayNightCycle = 120.0f;
    const float cyclePosition = std::fmod(worldClock, dayNightCycle);
    const bool wasNight = nightTime;
    nightTime = cyclePosition >= 72.0f;
    if (nightTime != wasNight) {
        std::cout << (nightTime ? "[World] Night has fallen. Talisman shelters are active.\n"
                                : "[World] Morning breaks. Town routines resume.\n");
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
        TryAdvanceOffscreenStory(character, i, dt, !moving && !nightTime);
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
