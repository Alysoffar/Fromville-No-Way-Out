#include "engine/renderer/GrassRenderer.h"

#include <cstdlib>
#include <iostream>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>

#include "engine/renderer/ShaderUtils.h"
#include "engine/physics/CollisionWorld.h"

struct GrassTemplateVertex {
    glm::vec3 position;
    glm::vec2 texcoord;
};

void GrassRenderer::init(const CollisionWorld* cw, float terrainMinY) {
    // Billboard template: upright quad
    // Bottom vertices at Y=0, top at Y=1.8
    // Width: 1.2 (half-width = 0.6)
    const float w = 0.6f;
    const float h = 1.8f;

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

    // Generate random instance positions on XZ plane
    std::vector<glm::vec3> instancePositions;
    instancePositions.reserve(GRASS_COUNT);
    std::srand(1337);
    
    std::cout << "[GrassRenderer] Grounding " << GRASS_COUNT << " grass instances...\n";

    for (int i = 0; i < GRASS_COUNT; ++i) {
        float x = (static_cast<float>(std::rand()) / RAND_MAX) * 1000.0f - 500.0f;
        float z = (static_cast<float>(std::rand()) / RAND_MAX) * 1000.0f - 500.0f;
        
        float y = terrainMinY;
        if (cw) {
            // Raycast down from high up to find the terrain height
            glm::vec3 rayOrigin(x, 1000.0f, z);
            glm::vec3 rayDir(0.0f, -1.0f, 0.0f);
            HitResult hit;
            if (cw->RaycastMap(rayOrigin, rayDir, 2000.0f, hit)) {
                y = rayOrigin.y - hit.t;
            }
        }
        
        instancePositions.push_back(glm::vec3(x, y, z));
    }

    // Create VAO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Template VBO
    glGenBuffers(1, &templateVBO);
    glBindBuffer(GL_ARRAY_BUFFER, templateVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(templateVerts), templateVerts, GL_STATIC_DRAW);

    // aPosition: loc 0
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GrassTemplateVertex), (void*)0);
    // aTexCoord: loc 1
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GrassTemplateVertex), (void*)(sizeof(float) * 3));

    // Instance VBO
    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, instancePositions.size() * sizeof(glm::vec3), instancePositions.data(), GL_STATIC_DRAW);

    // aInstancePos: loc 2, divisor 1
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glVertexAttribDivisor(2, 1);

    glBindVertexArray(0);

    // Load texture
    texGrass = loadTexture("textures/vegetation_grass_card_03.png");

    // Shaders
    shaderProgram = ShaderUtils::loadShaderProgram("shaders/grass.vert", "shaders/grass.frag");
}

void GrassRenderer::render(const glm::mat4& view, const glm::mat4& projection,
                           const glm::vec3& cameraPos, float currentTime,
                           const glm::vec3& lightDir, const glm::vec3& lightColor,
                           const glm::vec3& ambient, float diffuseStrength,
                           const glm::vec3& fogColor, float fogDensity) {
    if (shaderProgram == 0 || vao == 0) return;

    // Billboard matrix
    glm::vec3 cameraFront = -glm::vec3(view[0][2], view[1][2], view[2][2]);
    cameraFront.y = 0.0f;
    if (glm::length(cameraFront) > 0.001f) cameraFront = glm::normalize(cameraFront);
    else cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);

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

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texGrass);
    glUniform1i(glGetUniformLocation(shaderProgram, "uGrassTexture"), 0);

    GLboolean previousCull = glIsEnabled(GL_CULL_FACE);
    glDisable(GL_CULL_FACE);
    glBindVertexArray(vao);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, GRASS_COUNT);
    glBindVertexArray(0);
    if (previousCull) glEnable(GL_CULL_FACE);
}

void GrassRenderer::cleanup() {
    if (templateVBO != 0) glDeleteBuffers(1, &templateVBO);
    if (instanceVBO != 0) glDeleteBuffers(1, &instanceVBO);
    if (vao != 0) glDeleteVertexArrays(1, &vao);
    if (texGrass != 0) glDeleteTextures(1, &texGrass);
    if (shaderProgram != 0) glDeleteProgram(shaderProgram);
}

GLuint GrassRenderer::loadTexture(const char* path) {
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &width, &height, &channels, 0);
    if (!data) return 0;

    GLenum format = GL_RGBA;
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(data);
    return texID;
}
