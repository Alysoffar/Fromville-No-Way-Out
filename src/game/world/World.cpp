#include "game/world/World.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
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
        }
    }

    // ---- Map (Resident evil model, load synchronously for immediate display) ----
    if (!gVillageReady) {
        std::vector<MeshVertex> vertices;
        std::vector<unsigned int> indices;
        if (Loader::LoadOBJ("assets/models/Resident evil.obj", vertices, indices)) {
            gVillageMesh.Create(vertices, indices);
            gVillageShader.Load("assets/shaders/model_lit.vert", "assets/shaders/model_lit.frag");
            gVillageReady = gVillageMesh.IsValid();
            std::cout << "[World] Map (Resident evil) loaded synchronously!\n";
        } else {
            std::cerr << "Failed to load map OBJ: assets/models/Resident evil.obj\n";
        }
    }

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

    // Load collision geometry from the same map OBJ
    if (collisionWorld.LoadMap("assets/models/Resident evil.obj")) {
        std::cout << "[World] Collision map loaded successfully!\n";
    } else {
        std::cerr << "[World] Warning: Failed to load collision map, "
                  << "entities will use fallback physics.\n";
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
            if (enemy.HasKilledPlayer()) {
                playerKilled = true;
                std::cerr << "Enemy reached the player. The player is caught.\n";
            }
        }
    }

    if (activeChar) {
        previousActivePosition = activeChar->transform.position;
        hasPreviousActivePosition = true;
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

    // Render terrain and buildings
    if (gVillageReady) {
        const glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
        gVillageShader.Bind();
        gVillageShader.SetMat4("projection", projection);
        gVillageShader.SetMat4("view", view);
        gVillageShader.SetMat4("model", model);
        gVillageMesh.Draw();
        gVillageShader.Unbind();
    }

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
    characters.emplace_back(std::make_unique<Boyd>(glm::vec3(0.0f, 2.0f, 5.0f)));
    characters.emplace_back(std::make_unique<Jade>(glm::vec3(-3.0f, 2.0f, 5.0f)));
    characters.emplace_back(std::make_unique<Tabitha>(glm::vec3(3.0f, 2.0f, 5.0f)));
    characters.emplace_back(std::make_unique<Victor>(glm::vec3(-6.0f, 2.0f, 5.0f)));
    characters.emplace_back(std::make_unique<Sara>(glm::vec3(6.0f, 2.0f, 5.0f)));

    // Create NPCs
    if (npcs.empty()) {
        npcs.emplace_back("Mara", glm::vec3(-4.0f, 0.0f, 2.5f));
        npcs.emplace_back("Elena", glm::vec3(2.5f, 0.0f, 4.0f));
        npcs.emplace_back("Tom", glm::vec3(6.5f, 0.0f, -2.0f));
    }

    // Create enemies
    if (enemies.empty()) {
        enemies.emplace_back(glm::vec3(11.0f, 0.0f, 9.0f));
        enemies.emplace_back(glm::vec3(-10.0f, 0.0f, -7.5f));
    }
}

void World::InitializePlaceholderWorldRules() {
    if (storyLocations.empty()) {
        storyLocations.push_back({"sheriff_station", glm::vec3(0.0f, 0.0f, 8.0f), 5.0f});
        storyLocations.push_back({"diner", glm::vec3(-6.0f, 0.0f, 1.0f), 4.5f});
        storyLocations.push_back({"church", glm::vec3(8.0f, 0.0f, -2.0f), 4.5f});
        storyLocations.push_back({"tunnel_entrance", glm::vec3(-8.0f, 0.0f, -6.0f), 5.0f});
        storyLocations.push_back({"colony_house", glm::vec3(9.0f, 0.0f, 8.0f), 6.0f});
        storyLocations.push_back({"victor_hideout", glm::vec3(-9.0f, 0.0f, 5.0f), 4.0f});
    }

    if (shelterZones.empty()) {
        shelterZones.push_back({"sheriff_station_shelter", glm::vec3(0.0f, 0.0f, 8.0f), 5.5f, true, true});
        shelterZones.push_back({"diner_shelter", glm::vec3(-6.0f, 0.0f, 1.0f), 4.8f, true, true});
        shelterZones.push_back({"colony_house_shelter", glm::vec3(9.0f, 0.0f, 8.0f), 6.5f, true, true});
        shelterZones.push_back({"church_shelter", glm::vec3(8.0f, 0.0f, -2.0f), 4.8f, true, true});
    }

    std::cout << "[World] Placeholder story locations and talisman shelter zones initialized.\n";
}

bool World::TryActiveCharacterInteraction() {
    if (!questSystem) {
        return false;
    }

    Character* activeChar = GetActiveCharacter();
    if (!activeChar) {
        return false;
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
