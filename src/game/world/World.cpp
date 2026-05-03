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

// ---------- Async loading state for the village model ----------
struct PendingMeshData {
    std::vector<MeshVertex> vertices;
    std::vector<unsigned int> indices;
    bool success = false;
};

std::future<PendingMeshData> gVillageFuture;
std::atomic<bool> gVillageLoading{false};  // true while bg thread is working
bool gVillageLoadStarted = false;          // true once we kicked off the load

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

    // ---- Village (huge mesh, load asynchronously) ----
    if (!gVillageReady && !gVillageLoadStarted) {
        gVillageLoadStarted = true;
        gVillageLoading.store(true);

        std::cout << "[World] Starting async load of village model...\n";

        gVillageFuture = std::async(std::launch::async, []() -> PendingMeshData {
            PendingMeshData data;
            data.success = Loader::LoadOBJ("assets/models/Resident evil.obj",
                                           data.vertices, data.indices);
            gVillageLoading.store(false);
            return data;
        });
    }
}

// Call every frame to check if the async load finished and finalize on main thread
void FinalizeAsyncLoads() {
    if (gVillageReady || !gVillageLoadStarted) {
        return; // nothing to do
    }

    // Check if the future is ready (non-blocking)
    if (gVillageFuture.valid() &&
        gVillageFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {

        PendingMeshData data = gVillageFuture.get();

        if (data.success) {
            std::cout << "[World] Village OBJ parsed. Uploading to GPU ("
                      << data.vertices.size() << " verts, "
                      << data.indices.size() << " indices)...\n";

            gVillageMesh.Create(data.vertices, data.indices);
            gVillageShader.Load("assets/shaders/model_lit.vert", "assets/shaders/model_lit.frag");
            gVillageReady = gVillageMesh.IsValid();

            if (gVillageReady) {
                std::cout << "[World] Village model ready!\n";
            } else {
                std::cerr << "[World] Village mesh upload failed.\n";
            }
        } else {
            std::cerr << "[World] Failed to load village OBJ: assets/models/Resident evil.obj\n";
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
}

void World::Update(const Camera& camera, float dt) {
    if (!mapManager || !terrain) {
        return;
    }

    // Check if async model loading finished — must run on main thread for GL calls
    FinalizeAsyncLoads();

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