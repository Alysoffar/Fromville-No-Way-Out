#include "engine/renderer/TerrainRenderer.h"

#include <iostream>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>

#include "engine/renderer/ShaderUtils.h"

// Vertex layout: position (3), normal (3), texcoord (2) = 8 floats
struct GroundVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texcoord;
};

void GroundRenderer::init() {
    // Build a flat quad: 200x200 units centered at origin, Y = 0
    const float halfSize = 100.0f;
    GroundVertex vertices[6] = {
        // Triangle 1
        { glm::vec3(-halfSize, 0.0f, -halfSize), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 0.0f) },
        { glm::vec3( halfSize, 0.0f, -halfSize), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 0.0f) },
        { glm::vec3( halfSize, 0.0f,  halfSize), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 1.0f) },
        // Triangle 2
        { glm::vec3(-halfSize, 0.0f, -halfSize), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 0.0f) },
        { glm::vec3( halfSize, 0.0f,  halfSize), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 1.0f) },
        { glm::vec3(-halfSize, 0.0f,  halfSize), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 1.0f) },
    };

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Position: location 0
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GroundVertex),
                          reinterpret_cast<void*>(offsetof(GroundVertex, position)));
    // Normal: location 1
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GroundVertex),
                          reinterpret_cast<void*>(offsetof(GroundVertex, normal)));
    // Texcoord: location 2
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GroundVertex),
                          reinterpret_cast<void*>(offsetof(GroundVertex, texcoord)));

    glBindVertexArray(0);

    // Load textures
    texAlbedo    = loadTexture("assets/models/textures/coast_sand_rocks_02_diff_2k.jpg");
    texNormal    = loadTexture("assets/models/textures/coast_sand_rocks_02_nor_gl_2k.png");
    texRoughness = loadTexture("assets/models/textures/coast_sand_rocks_02_rough_2k.png");

    // Compile shaders
    shaderProgram = ShaderUtils::loadShaderProgram("shaders/ground.vert", "shaders/ground.frag");
    if (shaderProgram == 0) {
        std::cerr << "[GroundRenderer] Failed to load ground shaders!\n";
    }

    std::cout << "[GroundRenderer] Initialized successfully.\n";
}

GLuint GroundRenderer::loadTexture(const char* path) {
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &width, &height, &channels, 0);
    if (!data) {
        std::cerr << "[GroundRenderer] Failed to load texture: " << path << "\n";
        return 0;
    }

    GLenum format = GL_RGB;
    if (channels == 4) format = GL_RGBA;
    else if (channels == 1) format = GL_RED;

    GLuint texID = 0;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(format), width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);

    std::cout << "[GroundRenderer] Loaded texture: " << path
              << " (" << width << "x" << height << ", ch=" << channels << ")\n";
    return texID;
}

void GroundRenderer::render(const glm::mat4& view, const glm::mat4& projection,
                            const glm::vec3& cameraPos, const glm::vec3& lightDir,
                            const glm::vec3& lightColor, const glm::vec3& ambient,
                            float diffuseStrength, const glm::vec3& fogColor, float fogDensity) {
    if (shaderProgram == 0 || vao == 0) return;

    glUseProgram(shaderProgram);

    glm::mat4 model = glm::mat4(1.0f);

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uModel"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uView"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uProjection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform1f(glGetUniformLocation(shaderProgram, "uTilingFactor"), 80.0f);

    glUniform3fv(glGetUniformLocation(shaderProgram, "uLightDir"), 1, glm::value_ptr(lightDir));
    glUniform3fv(glGetUniformLocation(shaderProgram, "uLightColor"), 1, glm::value_ptr(lightColor));
    glUniform3fv(glGetUniformLocation(shaderProgram, "uAmbient"), 1, glm::value_ptr(ambient));
    glUniform1f(glGetUniformLocation(shaderProgram, "uDiffuseStrength"), diffuseStrength);
    glUniform3fv(glGetUniformLocation(shaderProgram, "uViewPos"), 1, glm::value_ptr(cameraPos));
    glUniform3fv(glGetUniformLocation(shaderProgram, "uFogColor"), 1, glm::value_ptr(fogColor));
    glUniform1f(glGetUniformLocation(shaderProgram, "uFogDensity"), fogDensity);

    // Bind textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texAlbedo);
    glUniform1i(glGetUniformLocation(shaderProgram, "uAlbedoMap"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texNormal);
    glUniform1i(glGetUniformLocation(shaderProgram, "uNormalMap"), 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, texRoughness);
    glUniform1i(glGetUniformLocation(shaderProgram, "uRoughnessMap"), 2);

    // Draw the quad
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glUseProgram(0);
}

void GroundRenderer::cleanup() {
    if (vbo != 0) { glDeleteBuffers(1, &vbo); vbo = 0; }
    if (vao != 0) { glDeleteVertexArrays(1, &vao); vao = 0; }
    if (texAlbedo != 0) { glDeleteTextures(1, &texAlbedo); texAlbedo = 0; }
    if (texNormal != 0) { glDeleteTextures(1, &texNormal); texNormal = 0; }
    if (texRoughness != 0) { glDeleteTextures(1, &texRoughness); texRoughness = 0; }
    if (shaderProgram != 0) { glDeleteProgram(shaderProgram); shaderProgram = 0; }
}
