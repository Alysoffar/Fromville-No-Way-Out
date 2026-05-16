#include "engine/renderer/SkydomeRenderer.h"

#include <cmath>
#include <iostream>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "engine/renderer/ShaderUtils.h"

void SkydomeRenderer::init() {
    const int stacks = 16;
    const int slices = 24;
    const float radius = 450.0f;

    std::vector<glm::vec3> vertices;
    std::vector<unsigned int> indices;

    for (int i = 0; i <= stacks; ++i) {
        float phi = static_cast<float>(i) / stacks * glm::pi<float>();
        float y = std::cos(phi) * radius;
        float r = std::sin(phi) * radius;
        for (int j = 0; j <= slices; ++j) {
            float theta = static_cast<float>(j) / slices * 2.0f * glm::pi<float>();
            vertices.push_back(glm::vec3(r * std::cos(theta), y, r * std::sin(theta)));
        }
    }

    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            unsigned int first = i * (slices + 1) + j;
            unsigned int second = first + slices + 1;
            indices.push_back(first); indices.push_back(second); indices.push_back(first + 1);
            indices.push_back(first + 1); indices.push_back(second); indices.push_back(second + 1);
        }
    }

    indexCount = static_cast<int>(indices.size());
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glBindVertexArray(0);

    shaderProgram = ShaderUtils::loadShaderProgram("shaders/sky.vert", "shaders/sky.frag");
}

void SkydomeRenderer::render(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& sunDir, float dayFactor, float dayTime) {
    if (shaderProgram == 0 || vao == 0) return;

    GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean cullFaceEnabled = glIsEnabled(GL_CULL_FACE);
    GLboolean depthMaskEnabled = GL_TRUE;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMaskEnabled);

    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glUseProgram(shaderProgram);
    glm::mat4 skyView = glm::mat4(glm::mat3(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uView"), 1, GL_FALSE, glm::value_ptr(skyView));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uProjection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uModel"), 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
    glUniform3fv(glGetUniformLocation(shaderProgram, "uSunDir"), 1, glm::value_ptr(sunDir));
    glUniform1f(glGetUniformLocation(shaderProgram, "uDayFactor"), dayFactor);
    glUniform1f(glGetUniformLocation(shaderProgram, "uDayTime"), dayTime);

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    if (cullFaceEnabled) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    if (depthTestEnabled) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    glDepthMask(depthMaskEnabled);
}

void SkydomeRenderer::cleanup() {
    if (vao) glDeleteVertexArrays(1, &vao);
    if (vbo) glDeleteBuffers(1, &vbo);
    if (ebo) glDeleteBuffers(1, &ebo);
    if (shaderProgram) glDeleteProgram(shaderProgram);
}
