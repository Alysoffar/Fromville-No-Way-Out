#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <vector>

class Shader;

class Terrain {
public:
    Terrain(int gridWidth, int gridDepth, float cellSize);
    ~Terrain();

    void Generate();
    void Draw(Shader& shader) const;
    float GetHeightAt(float x, float z) const;

private:
    int gridW;
    int gridD;
    float cell;

    std::vector<float> heightMap;
    GLuint grassTexture = 0;
    GLuint dirtTexture = 0;
    GLuint rockTexture = 0;

    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    int indexCount = 0;

    struct TerrainVert {
        glm::vec3 pos;
        glm::vec3 nrm;
        glm::vec2 uv;
    };

    std::vector<TerrainVert> verts;
    std::vector<unsigned int> inds;

    void GenerateHeightMap();
    void BuildMesh();
};
