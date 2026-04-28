#include "game/world/World.h"

#include <cmath>
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

Mesh gCottageMesh;
Shader gCottageShader("CottageModel");
bool gCottageReady = false;

Mesh gNpcMesh;
Mesh gEnemyMesh;
Shader gCharacterShader("CharacterCubes");
bool gCharacterReady = false;

constexpr float kNpcSightRange = 9.0f;
constexpr float kEnemySightRange = 18.0f;

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

glm::vec3 FindNearestTargetPosition(const std::vector<NPC>& npcs, const glm::vec3& enemyPosition, float sightRange, bool& visibleOut) {
    visibleOut = false;
    if (npcs.empty()) {
        return enemyPosition;
    }

    float bestDistance = sightRange;
    glm::vec3 bestPosition = enemyPosition;

    for (const NPC& npc : npcs) {
        glm::vec3 delta = npc.transform.position - enemyPosition;
        delta.y = 0.0f;
        const float distance = glm::length(delta);
        if (distance < bestDistance) {
            bestDistance = distance;
            bestPosition = npc.transform.position;
            visibleOut = true;
        }
    }

    return bestPosition;
}

void InitializeModels() {
    std::vector<MeshVertex> vertices;
    std::vector<unsigned int> indices;

    if (!gPlayerReady) {
        if (Loader::LoadOBJ("assets/models/boyd.obj", vertices, indices)) {
            gPlayerMesh.Create(vertices, indices);
            gPlayerShader.Load("assets/shaders/model_lit.vert", "assets/shaders/model_lit.frag");
            gPlayerReady = gPlayerMesh.IsValid();
        } else {
            std::cerr << "Failed to load player OBJ: assets/models/boyd.obj\n";
        }
    }

    vertices.clear();
    indices.clear();

    if (!gCottageReady) {
        if (Loader::LoadOBJ("assets/models/cottage_obj.obj", vertices, indices)) {
            gCottageMesh.Create(vertices, indices);
            gCottageShader.Load("assets/shaders/model_lit.vert", "assets/shaders/model_lit.frag");
            gCottageReady = gCottageMesh.IsValid();
        } else {
            std::cerr << "Failed to load cottage OBJ: assets/models/cottage_obj.obj\n";
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
}

void World::Update(const Camera& camera, float dt) {
    if (!mapManager || !terrain) {
        return;
    }

    UpdateTimeOfDay(dt);

    player.Update(dt);

    for (NPC& npc : npcs) {
        npc.SetNight(nightTime);
        bool enemyVisible = false;
        const glm::vec3 threatPosition = FindNearestEnemyPosition(enemies, npc.transform.position, kNpcSightRange, enemyVisible);
        npc.SetThreatPosition(threatPosition, enemyVisible);
        npc.Update(dt);
    }

    if (!playerKilled) {
        for (Enemy& enemy : enemies) {
            bool targetVisible = false;
            const glm::vec3 nearestNpcPosition = FindNearestTargetPosition(npcs, enemy.transform.position, kEnemySightRange, targetVisible);
            if (targetVisible) {
                enemy.SetVisibleTarget(nearestNpcPosition, true);
            } else {
                const glm::vec3 playerDelta = player.transform.position - enemy.transform.position;
                const float playerDistance = glm::length(glm::vec3(playerDelta.x, 0.0f, playerDelta.z));
                const bool playerVisible = playerDistance <= kEnemySightRange;
                enemy.SetVisibleTarget(player.transform.position, playerVisible);
            }
            enemy.Update(dt);
            if (enemy.HasKilledPlayer()) {
                playerKilled = true;
                std::cerr << "Enemy reached the player. The player is caught.\n";
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

    if (gPlayerReady) {
        const glm::mat4 model = player.transform.GetMatrix();
        gPlayerShader.Bind();
        gPlayerShader.SetMat4("projection", projection);
        gPlayerShader.SetMat4("view", view);
        gPlayerShader.SetMat4("model", model);
        gPlayerMesh.Draw();
        gPlayerShader.Unbind();
    }

    if (gCottageReady) {
        const glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(5.0f, 0.0f, -5.0f));
        gCottageShader.Bind();
        gCottageShader.SetMat4("projection", projection);
        gCottageShader.SetMat4("view", view);
        gCottageShader.SetMat4("model", model);
        gCottageMesh.Draw();
        gCottageShader.Unbind();
    }

    if (gCharacterReady) {
        for (const NPC& npc : npcs) {
            RenderCharacterCube(gCharacterShader, gNpcMesh, camera, aspectRatio, npc.transform.position, glm::vec3(0.55f, 1.0f, 0.55f), npc.GetDebugColor());
        }

        for (const Enemy& enemy : enemies) {
            RenderCharacterCube(gCharacterShader, gEnemyMesh, camera, aspectRatio, enemy.transform.position, glm::vec3(0.65f, 1.15f, 0.65f), enemy.GetDebugColor());
        }
    }
}

void World::InitializeCharacters() {
    if (!npcs.empty() || !enemies.empty()) {
        return;
    }

    npcs.emplace_back("Mara", glm::vec3(-4.0f, 0.0f, 2.5f));
    npcs.emplace_back("Elena", glm::vec3(2.5f, 0.0f, 4.0f));
    npcs.emplace_back("Tom", glm::vec3(6.5f, 0.0f, -2.0f));

    enemies.emplace_back(glm::vec3(11.0f, 0.0f, 9.0f));
    enemies.emplace_back(glm::vec3(-10.0f, 0.0f, -7.5f));
}

void World::UpdateTimeOfDay(float dt) {
    worldClock += dt;
    constexpr float dayNightCycle = 120.0f;
    const float cyclePosition = std::fmod(worldClock, dayNightCycle);
    nightTime = cyclePosition >= 72.0f;
}