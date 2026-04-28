#include "game/world/World.h"

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

    if (gCottageReady) {
        const glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(5.0f, 0.0f, -5.0f));
        gCottageShader.Bind();
        gCottageShader.SetMat4("projection", projection);
        gCottageShader.SetMat4("view", view);
        gCottageShader.SetMat4("model", model);
        gCottageMesh.Draw();
        gCottageShader.Unbind();
    }
}