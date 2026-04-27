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
Mesh gHouseMesh;
Shader gHouseShader("HouseModel");
bool gHouseReady = false;

void InitializeHouseModel() {
    if (gHouseReady) {
        return;
    }

    std::vector<MeshVertex> vertices;
    std::vector<unsigned int> indices;
    if (!Loader::LoadOBJ("assets/models/cottage_obj.obj", vertices, indices)) {
        std::cerr << "Failed to load house OBJ: assets/models/cottage_obj.obj\n";
        return;
    }

    gHouseMesh.Create(vertices, indices);
    gHouseShader.Load("assets/shaders/basic_cube.vert", "assets/shaders/basic_cube.frag");
    gHouseReady = gHouseMesh.IsValid();
}
}

World::World() = default;

void World::Initialize() {
    static MapManager worldMap;
    static TerrainRenderer terrainRenderer;

    mapManager = &worldMap;
    terrain = &terrainRenderer;

    terrain->Initialize();
    InitializeHouseModel();
}

void World::Update(const Camera& camera, float dt) {
    (void)dt;

    if (!mapManager || !terrain) {
        return;
    }

    mapManager->Update(camera.GetPosition());
    terrain->Update(*mapManager);
}

void World::Render(const Camera& camera, float aspectRatio) {
    if (!terrain) {
        return;
    }

    terrain->Render(camera, aspectRatio);

    if (!gHouseReady) {
        return;
    }

    const glm::mat4 projection = camera.GetProjectionMatrix(aspectRatio);
    const glm::mat4 view = camera.GetViewMatrix();
    const glm::mat4 model =
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.3f, -8.0f)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(1.0f));

    gHouseShader.Bind();
    gHouseShader.SetMat4("projection", projection);
    gHouseShader.SetMat4("view", view);
    gHouseShader.SetMat4("model", model);
    // Choose a warm, slightly desaturated color so the house contrasts with the white floor
    gHouseShader.SetVec3("color", glm::vec3(0.55f, 0.42f, 0.31f));
    gHouseMesh.Draw();
    gHouseShader.Unbind();
}