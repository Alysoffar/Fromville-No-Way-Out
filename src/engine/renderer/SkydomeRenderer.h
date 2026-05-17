#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

class SkydomeRenderer {
public:
    SkydomeRenderer() = default;

    void init();
    void render(const glm::mat4& view, const glm::mat4& projection,
                const glm::vec3& sunDir, float dayFactor, float dayTime);
    void cleanup();

private:
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLuint shaderProgram = 0;
    int indexCount = 0;
};
