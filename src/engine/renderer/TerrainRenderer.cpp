#include "engine/renderer/TerrainRenderer.h"

#include <iostream>
#include <vector>
#include <float.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>

#include "engine/renderer/Mesh.h"
#include "engine/renderer/ShaderUtils.h"

void TerrainRenderer::init() {
    minY = 0.0f;

    // Create a large flat quad centered at origin
    std::vector<MeshVertex> vertices = {
        {{-500.0f, 0.0f, -500.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
        {{ 500.0f, 0.0f, -500.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
        {{ 500.0f, 0.0f,  500.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
        {{-500.0f, 0.0f, -500.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
        {{ 500.0f, 0.0f,  500.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
        {{-500.0f, 0.0f,  500.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
    };

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(MeshVertex), vertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex, normal));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex, uv));

    glBindVertexArray(0);

    // ---- Load Textures ----
    texAlbedo    = loadTexture("textures/coast_sand_rocks_02_diff_2k.jpg");
    texNormal    = loadTexture("textures/coast_sand_rocks_02_nor_gl_2k.png");
    texRoughness = loadTexture("textures/coast_sand_rocks_02_rough_2k.png");

    // ---- Compile Shaders ----
    shaderProgram = ShaderUtils::loadShaderProgram("shaders/ground.vert", "shaders/ground.frag");

    std::cout << "[TerrainRenderer] Initialized flat ground quad with tiled textures\n";
}

void TerrainRenderer::render(const glm::mat4& view, const glm::mat4& projection,
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

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texAlbedo);
    glUniform1i(glGetUniformLocation(shaderProgram, "uAlbedoMap"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texNormal);
    glUniform1i(glGetUniformLocation(shaderProgram, "uNormalMap"), 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, texRoughness);
    glUniform1i(glGetUniformLocation(shaderProgram, "uRoughnessMap"), 2);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glUseProgram(0);
}

void TerrainRenderer::cleanup() {
    if (vbo != 0) glDeleteBuffers(1, &vbo);
    if (ebo != 0) glDeleteBuffers(1, &ebo);
    if (vao != 0) glDeleteVertexArrays(1, &vao);
    if (texAlbedo != 0) glDeleteTextures(1, &texAlbedo);
    if (texNormal != 0) glDeleteTextures(1, &texNormal);
    if (texRoughness != 0) glDeleteTextures(1, &texRoughness);
    if (shaderProgram != 0) glDeleteProgram(shaderProgram);
}

GLuint TerrainRenderer::loadTexture(const char* path) {
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &width, &height, &channels, 0);
    if (!data) {
        std::cerr << "[TerrainRenderer] Failed to load texture: " << path << "\n";
        return 0;
    }

    GLenum format = GL_RGB;
    if (channels == 4) format = GL_RGBA;
    else if (channels == 1) format = GL_RED;

    GLuint texID = 0;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    return texID;
}
