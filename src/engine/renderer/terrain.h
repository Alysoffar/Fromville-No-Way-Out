#pragma once

#include <vector>

#include "engine/renderer/Mesh.h"
#include "engine/renderer/Shader.h"
#include "engine/physics/Triangle.h"

class MapManager;
class Camera;

class TerrainRenderer {
public:
    TerrainRenderer() = default;

    void Initialize();
    void Update(const MapManager& mapManager);
    void Render(const Camera& camera, float aspectRatio);

    std::vector<Triangle> GetTriangles() const;
    Shader& GetShader() { return shader; }

private:
    void RebuildMesh();
    void RebuildMountainMesh();

    Shader shader{"Terrain"};
    Shader skyShader{"Sky"};
    unsigned int skyVAO = 0;
    
    Mesh mesh;
    Mesh mountainMesh;
    bool initialized = false;
    std::vector<Triangle> collisionTriangles;
};