#include "engine/renderer/GrassRenderer.h"

#include <cstdlib>
#include <iostream>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>

#include "engine/renderer/ShaderUtils.h"

struct GrassTemplateVertex {
    glm::vec3 position;
    glm::vec2 texcoord;
};

void GrassRenderer::init() {
    // Billboard template: upright quad
    // Bottom vertices at Y=0, top at Y=h
    // UV: V range 0.0 to 0.8 to avoid empty top strip
    const float w = 0.35f;  // half-width
    const float h = 0.9f;

    GrassTemplateVertex templateVerts[6] = {
        // Triangle 1
        { glm::vec3(-w, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f) },
        { glm::vec3( w, 0.0f, 0.0f), glm::vec2(1.0f, 0.0f) },
        { glm::vec3( w, h,    0.0f), glm::vec2(1.0f, 0.8f) },
        // Triangle 2
        { glm::vec3(-w, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f) },
        { glm::vec3( w, h,    0.0f), glm::vec2(1.0f, 0.8f) },
        { glm::vec3(-w, h,    0.0f), glm::vec2(0.0f, 0.8f) },
    };

    // Generate random instance positions on XZ plane within the ground area
    std::vector<glm::vec3> instancePositions(GRASS_COUNT);
    std::srand(42);
    for (int i = 0; i < GRASS_COUNT; ++i) {
        float x, z;
        bool inHouseArea = true;
        while (inHouseArea) {
            x = (static_cast<float>(std::rand()) / RAND_MAX) * 200.0f - 100.0f;
            z = (static_cast<float>(std::rand()) / RAND_MAX) * 200.0f - 100.0f;

            bool inAnyHouse = false;
            glm::vec2 houseCenters[] = {
                {-29.0f, 0.0f},
                {29.0f, 0.0f},
                {-29.0f, 20.0f},
                {29.0f, 20.0f},
                {-29.0f, -20.0f},
                {29.0f, -20.0f},
                {0.0f, 24.0f},
                {9.0f, 12.0f}
            };
            for (const auto& center : houseCenters) {
                if (glm::distance(glm::vec2(x, z), center) < 17.0f) {
                    inAnyHouse = true;
                    break;
                }
            }
            bool inDinner = glm::distance(glm::vec2(x, z), glm::vec2(-19.32f, -42.84f)) < 17.18f;
            bool inPolice = glm::distance(glm::vec2(x, z), glm::vec2(-65.84f, 15.84f)) < 11.3f;

            if (!inAnyHouse && !inDinner && !inPolice) {
                inHouseArea = false;
            }
        }
        instancePositions[i] = glm::vec3(x, 0.0f, z);
    }

    // Create VAO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Template VBO (per-vertex data)
    glGenBuffers(1, &templateVBO);
    glBindBuffer(GL_ARRAY_BUFFER, templateVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(templateVerts), templateVerts, GL_STATIC_DRAW);

    // Position: location 0
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GrassTemplateVertex),
                          reinterpret_cast<void*>(offsetof(GrassTemplateVertex, position)));
    // Texcoord: location 1
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GrassTemplateVertex),
                          reinterpret_cast<void*>(offsetof(GrassTemplateVertex, texcoord)));

    // Instance VBO (per-instance world position)
    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(instancePositions.size() * sizeof(glm::vec3)),
                 instancePositions.data(), GL_STATIC_DRAW);

    // Instance position: location 2, divisor 1
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), reinterpret_cast<void*>(0));
    glVertexAttribDivisor(2, 1);

    glBindVertexArray(0);

    // Load grass texture
    texGrass = loadTexture("assets/models/vegetation_grass_card_03.png");

    // Compile shaders
    shaderProgram = ShaderUtils::loadShaderProgram("shaders/grass.vert", "shaders/grass.frag");
    if (shaderProgram == 0) {
        std::cerr << "[GrassRenderer] Failed to load grass shaders!\n";
    }

    std::cout << "[GrassRenderer] Initialized with " << GRASS_COUNT << " instances.\n";
}

GLuint GrassRenderer::loadTexture(const char* path) {
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &width, &height, &channels, 0);
    if (!data) {
        std::cerr << "[GrassRenderer] Failed to load texture: " << path << "\n";
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

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);

    std::cout << "[GrassRenderer] Loaded texture: " << path
              << " (" << width << "x" << height << ", ch=" << channels << ")\n";
    return texID;
}

void GrassRenderer::render(const glm::mat4& view, const glm::mat4& projection,
                           const glm::vec3& cameraPos, float currentTime,
                           const glm::vec3& lightDir, const glm::vec3& lightColor,
                           const glm::vec3& ambient, float diffuseStrength,
                           const glm::vec3& fogColor, float fogDensity) {
    if (shaderProgram == 0 || vao == 0) return;

    // Compute billboard matrix on CPU
    // Camera front from view matrix (negative of third row)
    glm::vec3 cameraFront = -glm::vec3(view[0][2], view[1][2], view[2][2]);
    cameraFront.y = 0.0f;
    if (glm::length(cameraFront) > 0.001f) {
        cameraFront = glm::normalize(cameraFront);
    } else {
        cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    }

    glm::vec3 bbRight = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), cameraFront));
    glm::vec3 bbUp    = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 bbLook  = glm::normalize(glm::cross(bbRight, bbUp));

    glm::mat3 billboard(bbRight, bbUp, bbLook);

    glUseProgram(shaderProgram);

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uView"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uProjection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform1f(glGetUniformLocation(shaderProgram, "uTime"), currentTime);
    glUniformMatrix3fv(glGetUniformLocation(shaderProgram, "uBillboard"), 1, GL_FALSE, glm::value_ptr(billboard));

    glUniform3fv(glGetUniformLocation(shaderProgram, "uLightDir"), 1, glm::value_ptr(lightDir));
    glUniform3fv(glGetUniformLocation(shaderProgram, "uLightColor"), 1, glm::value_ptr(lightColor));
    glUniform3fv(glGetUniformLocation(shaderProgram, "uAmbient"), 1, glm::value_ptr(ambient));
    glUniform1f(glGetUniformLocation(shaderProgram, "uDiffuseStrength"), diffuseStrength);
    glUniform3fv(glGetUniformLocation(shaderProgram, "uViewPos"), 1, glm::value_ptr(cameraPos));
    glUniform3fv(glGetUniformLocation(shaderProgram, "uFogColor"), 1, glm::value_ptr(fogColor));
    glUniform1f(glGetUniformLocation(shaderProgram, "uFogDensity"), fogDensity);

    // Bind grass texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texGrass);
    glUniform1i(glGetUniformLocation(shaderProgram, "uGrassTexture"), 0);

    // Disable back-face culling for grass billboards
    GLboolean cullWasEnabled = glIsEnabled(GL_CULL_FACE);
    glDisable(GL_CULL_FACE);

    // Draw instanced
    glBindVertexArray(vao);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, GRASS_COUNT);
    glBindVertexArray(0);

    // Restore cull face state
    if (cullWasEnabled) {
        glEnable(GL_CULL_FACE);
    }

    glUseProgram(0);
}

void GrassRenderer::cleanup() {
    if (templateVBO != 0)  { glDeleteBuffers(1, &templateVBO); templateVBO = 0; }
    if (instanceVBO != 0)  { glDeleteBuffers(1, &instanceVBO); instanceVBO = 0; }
    if (vao != 0)          { glDeleteVertexArrays(1, &vao); vao = 0; }
    if (texGrass != 0)     { glDeleteTextures(1, &texGrass); texGrass = 0; }
    if (shaderProgram != 0){ glDeleteProgram(shaderProgram); shaderProgram = 0; }
}
