#include "game/world/World.h"

#include <atomic>
#include <future>
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


void InitializeModels() {
    // ---- Player (small mesh, load synchronously) ----
    if (!gPlayerReady) {
        std::vector<MeshVertex> vertices;
        std::vector<unsigned int> indices;
        if (Loader::LoadOBJ("assets/models/boyd.obj", vertices, indices)) {
            gPlayerMesh.Create(vertices, indices);
            gPlayerShader.Load("assets/shaders/model_lit.vert", "assets/shaders/model_lit.frag");
            gPlayerReady = gPlayerMesh.IsValid();
        } else {
            std::cerr << "Failed to load player OBJ: assets/models/boyd.obj\n";
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

}
}

World::World() = default;

void World::Initialize() {
    static MapManager worldMap;
    static TerrainRenderer terrainRenderer;

    mapManager = &worldMap;
    terrain = &terrainRenderer;

    terrain->Initialize();
    player.transform.position = glm::vec3(0.0f, 2.0f, 5.0f);
    InitializeModels();

    // Load collision geometry from the same map OBJ
    if (collisionWorld.LoadMap("assets/models/Resident evil.obj")) {
        std::cout << "[World] Collision map loaded successfully!\n";
    } else {
        std::cerr << "[World] Warning: Failed to load collision map, "
                  << "entities will use fallback physics.\n";
    }

    // Wire collision system to entities
    player.SetCollisionWorld(&collisionWorld);
}


void World::Update(const Camera& camera, float dt) {
    if (!mapManager || !terrain) {
        return;
    }

    player.Update(dt);
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

    if (gVillageReady) {
        const glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
        gVillageShader.Bind();
        gVillageShader.SetMat4("projection", projection);
        gVillageShader.SetMat4("view", view);
        gVillageShader.SetMat4("model", model);
        gVillageMesh.Draw();
        gVillageShader.Unbind();
    }
}