#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

class GroundRenderer {
public:
    GroundRenderer() = default;

    void init();
    void render(const glm::mat4& view, const glm::mat4& projection,
                const glm::vec3& cameraPos, const glm::vec3& lightDir,
                const glm::vec3& lightColor, const glm::vec3& ambient,
                float diffuseStrength, const glm::vec3& fogColor, float fogDensity);
    void cleanup();

private:
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint shaderProgram = 0;
    GLuint texAlbedo = 0;
    GLuint texNormal = 0;
    GLuint texRoughness = 0;

    GLuint loadTexture(const char* path);
};
