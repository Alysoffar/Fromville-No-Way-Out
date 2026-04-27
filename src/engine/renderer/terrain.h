#pragma once

#include <vector>

#include "engine/renderer/Mesh.h"
#include "engine/renderer/Shader.h"

class MapManager;
class Camera;

class TerrainRenderer {
public:
    TerrainRenderer() = default;

    void Initialize();
    void Update(const MapManager& mapManager);
    void Render(const Camera& camera, float aspectRatio);

private:
    void RebuildMesh(const MapManager& mapManager);

    Shader shader{"Terrain"};
    Mesh mesh;
    glm::vec2 lastOrigin = glm::vec2(1.0e9f);
    bool initialized = false;
};