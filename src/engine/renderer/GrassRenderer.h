#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

class GrassRenderer {
public:
    GrassRenderer() = default;

    void init();
    void render(const glm::mat4& view, const glm::mat4& projection,
                const glm::vec3& cameraPos, float currentTime,
                const glm::vec3& lightDir, const glm::vec3& lightColor,
                const glm::vec3& ambient, float diffuseStrength,
                const glm::vec3& fogColor, float fogDensity);
    void cleanup();

private:
    GLuint vao = 0;
    GLuint templateVBO = 0;
    GLuint instanceVBO = 0;
    GLuint shaderProgram = 0;
    GLuint texGrass = 0;

    static constexpr int GRASS_COUNT = 20000;

    GLuint loadTexture(const char* path);
};
