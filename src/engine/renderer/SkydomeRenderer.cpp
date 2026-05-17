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
    const float radius = 150.0f;

    std::vector<glm::vec3> vertices;
    std::vector<unsigned int> indices;

    // Generate sphere vertices
    for (int i = 0; i <= stacks; ++i) {
        float phi = static_cast<float>(i) / static_cast<float>(stacks) * glm::pi<float>();
        float y  = std::cos(phi) * radius;
        float r  = std::sin(phi) * radius;

        for (int j = 0; j <= slices; ++j) {
            float theta = static_cast<float>(j) / static_cast<float>(slices) * 2.0f * glm::pi<float>();
            float x = r * std::cos(theta);
            float z = r * std::sin(theta);
            vertices.push_back(glm::vec3(x, y, z));
        }
    }

    // Generate indices
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            unsigned int topLeft     = static_cast<unsigned int>(i * (slices + 1) + j);
            unsigned int topRight    = topLeft + 1;
            unsigned int bottomLeft  = static_cast<unsigned int>((i + 1) * (slices + 1) + j);
            unsigned int bottomRight = bottomLeft + 1;

            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    indexCount = static_cast<int>(indices.size());

    // Upload to GPU
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices.size() * sizeof(glm::vec3)),
                 vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
                 indices.data(), GL_STATIC_DRAW);

    // Position: location 0
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), reinterpret_cast<void*>(0));

    glBindVertexArray(0);

    // Compile sky shaders
    shaderProgram = ShaderUtils::loadShaderProgram("shaders/sky.vert", "shaders/sky.frag");
    if (shaderProgram == 0) {
        std::cerr << "[SkydomeRenderer] Failed to load sky shaders!\n";
    }

    std::cout << "[SkydomeRenderer] Initialized (" << stacks << " stacks, "
              << slices << " slices, " << indexCount << " indices).\n";
}

void SkydomeRenderer::render(const glm::mat4& view, const glm::mat4& projection,
                             const glm::vec3& sunDir, float dayFactor, float dayTime) {
    if (shaderProgram == 0 || vao == 0) return;

    // Save GL state
    GLboolean depthWriteWas;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthWriteWas);
    GLboolean depthTestWas = glIsEnabled(GL_DEPTH_TEST);
    GLboolean cullWas = glIsEnabled(GL_CULL_FACE);

    // Disable depth write & depth test for sky, disable cull face
    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    // Strip translation from view matrix (sky is always centered on camera)
    glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(view));
    glm::mat4 model = glm::mat4(1.0f);

    glUseProgram(shaderProgram);

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uProjection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uView"), 1, GL_FALSE, glm::value_ptr(viewNoTranslation));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uModel"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(glGetUniformLocation(shaderProgram, "uSunDir"), 1, glm::value_ptr(sunDir));
    glUniform1f(glGetUniformLocation(shaderProgram, "uDayFactor"), dayFactor);
    glUniform1f(glGetUniformLocation(shaderProgram, "uDayTime"), dayTime);

    // Draw indexed sphere
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    glUseProgram(0);

    // Restore GL state
    glDepthMask(depthWriteWas);
    if (depthTestWas) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (cullWas) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
}

void SkydomeRenderer::cleanup() {
    if (ebo != 0) { glDeleteBuffers(1, &ebo); ebo = 0; }
    if (vbo != 0) { glDeleteBuffers(1, &vbo); vbo = 0; }
    if (vao != 0) { glDeleteVertexArrays(1, &vao); vao = 0; }
    if (shaderProgram != 0) { glDeleteProgram(shaderProgram); shaderProgram = 0; }
    indexCount = 0;
}
