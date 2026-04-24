#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <vector>

class Shader;
class Terrain;

enum class RoadType {
    TARMAC,
    GRAVEL,
    DIRT,
};

class RoadNetwork {
public:
    ~RoadNetwork();
    void Build(Terrain& terrain);
    void Draw(Shader& shader) const;

private:
    struct RoadSegment {
        glm::vec3 start;
        glm::vec3 end;
        float width;
        RoadType type;
    };

    struct RoadVert {
        glm::vec3 pos;
        glm::vec3 nrm;
        glm::vec2 uv;
    };

    struct RoadBatch {
        GLuint vao = 0;
        GLuint vbo = 0;
        GLuint ebo = 0;
        int indexCount = 0;
        glm::vec3 albedo = glm::vec3(0.2f);
        float roughness = 0.9f;
    };

    std::vector<RoadSegment> segments;
    std::vector<RoadBatch> batches;

    void ClearBatches();
    void GenerateSegmentMesh(const RoadSegment& seg, std::vector<RoadVert>& verts, std::vector<unsigned int>& inds);
    void GenerateRoadMarkings(const RoadSegment& seg, std::vector<RoadVert>& verts, std::vector<unsigned int>& inds);
    void ApplyTerrainHeights(Terrain& terrain, std::vector<RoadVert>& verts, float heightBias);
    void UploadBatch(const std::vector<RoadVert>& verts,
                     const std::vector<unsigned int>& inds,
                     glm::vec3 albedo,
                     float roughness);
};
