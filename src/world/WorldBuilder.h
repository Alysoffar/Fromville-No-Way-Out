#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <memory>
#include <vector>

class NavMesh;
class TalismanSystem;
class Renderer;
class Camera;
class DayNightCycle;
class Shader;
class Terrain;
class RoadNetwork;

struct WorldSceneVertex {
    glm::vec3 pos = glm::vec3(0.0f);
    glm::vec3 nrm = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec2 uv = glm::vec2(0.0f);
};

struct WorldSceneBatch {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLuint textureID = 0;
    int indexCount = 0;
    glm::vec3 albedo = glm::vec3(0.5f);
    float roughness = 0.8f;
    float metalness = 0.0f;
    glm::vec3 emission = glm::vec3(0.0f);
};

class WorldBuilder {
public:
    WorldBuilder(NavMesh* navMesh, TalismanSystem* talismans);
    ~WorldBuilder();

    void BuildAll();
    void DrawAll(Renderer& renderer, Camera& camera, DayNightCycle& dayNight);
    float GetHeightAt(float x, float z) const;

private:
    NavMesh* nav = nullptr;
    TalismanSystem* talismanSystem = nullptr;

    std::unique_ptr<Terrain> terrain;
    std::unique_ptr<RoadNetwork> roads;
    std::unique_ptr<Shader> worldShader;
    std::vector<WorldSceneBatch> sceneBatches;
    bool shaderReady = false;

    void BuildTerrain();
    void BuildRoadNetwork();
    void BuildColonyHouse();
    void BuildSurvivorCabins();
    void BuildLandmarkBuildings();
    void BuildForestInterior();
    void BuildForestBoundary();
    void BuildDistantRidges();
    void BuildWhiteTreeGroves();
    void BuildBigRedMarker();
    void BuildAtmosphericProps();
    void BuildBottleTreeShrine();
    void BuildTownFacades();
    void BuildFencedLots();
    void BuildTunnelEntrances();
    void BuildSheriffStation();
    void BuildWaterTower();
    void BuildStoneCircles();
    void BuildSky();
    void BuildFog();
    void ClearSceneBatches();
    void UploadBatch(const std::vector<WorldSceneVertex>& verts,
                     const std::vector<unsigned int>& inds,
                     glm::vec3 albedo,
                     float roughness,
                     GLuint textureID = 0,
                     float metalness = 0.0f,
                     glm::vec3 emission = glm::vec3(0.0f));
};
