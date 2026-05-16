#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

class TerrainRenderer {
public:
    TerrainRenderer() = default;

    void init();
    void render(const glm::mat4& view, const glm::mat4& projection,
                const glm::vec3& cameraPos, const glm::vec3& lightDir,
                const glm::vec3& lightColor, const glm::vec3& ambient,
                float diffuseStrength, const glm::vec3& fogColor, float fogDensity);
    void cleanup();

    float getMinY() const { return minY; }

private:
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLuint shaderProgram = 0;
    GLuint texAlbedo = 0;
    GLuint texNormal = 0;
    GLuint texRoughness = 0;
    int indexCount = 0;
    float minY = 0.0f;

    GLuint loadTexture(const char* path);
};
