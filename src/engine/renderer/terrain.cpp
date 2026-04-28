#include "engine/renderer/terrain.h"

#include <cmath>

#include <glm/gtc/matrix_transform.hpp>

#include "game/world/map_manager.h"
#include "engine/renderer/Camera.h"

void TerrainRenderer::Initialize() {
    if (initialized) {
        return;
    }

    shader.Load("assets/shaders/basic_cube.vert", "assets/shaders/basic_cube.frag");
    initialized = true;
}

void TerrainRenderer::Update(const MapManager& mapManager) {
    if (!initialized) {
        Initialize();
    }

    const glm::vec2 origin = mapManager.GetOrigin();
    if (origin.x != lastOrigin.x || origin.y != lastOrigin.y) {
        RebuildMesh(mapManager);
        lastOrigin = origin;
    }
}

void TerrainRenderer::Render(const Camera& camera, float aspectRatio) {
    if (!initialized || !mesh.IsValid()) {
        return;
    }

    const glm::mat4 projection = camera.GetProjectionMatrix(aspectRatio);
    const glm::mat4 view = camera.GetViewMatrix();
    const glm::mat4 model = glm::mat4(1.0f);

    shader.Bind();
    shader.SetMat4("projection", projection);
    shader.SetMat4("view", view);
    shader.SetMat4("model", model);
    shader.SetVec3("color", glm::vec3(0.78f, 0.83f, 0.70f));

    mesh.Draw();

    shader.Unbind();
}

void TerrainRenderer::RebuildMesh(const MapManager& mapManager) {
    std::vector<MeshVertex> vertices;
    std::vector<unsigned int> indices;

    const int resolution = mapManager.GetResolution();
    const float spacing = mapManager.GetCellSize();
    const glm::vec2 origin = mapManager.GetOrigin();

    vertices.reserve(static_cast<std::size_t>(resolution * resolution));
    indices.reserve(static_cast<std::size_t>((resolution - 1) * (resolution - 1) * 6));

    for (int z = 0; z < resolution; ++z) {
        for (int x = 0; x < resolution; ++x) {
            const float worldX = origin.x + static_cast<float>(x) * spacing;
            const float worldZ = origin.y + static_cast<float>(z) * spacing;
            MeshVertex vertex;
            vertex.position = glm::vec3(worldX, mapManager.SampleHeight(worldX, worldZ), worldZ);
            vertices.push_back(vertex);
        }
    }

    for (int z = 0; z < resolution - 1; ++z) {
        for (int x = 0; x < resolution - 1; ++x) {
            const unsigned int topLeft = static_cast<unsigned int>(z * resolution + x);
            const unsigned int topRight = topLeft + 1;
            const unsigned int bottomLeft = static_cast<unsigned int>((z + 1) * resolution + x);
            const unsigned int bottomRight = bottomLeft + 1;

            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    mesh.Create(vertices, indices);
}