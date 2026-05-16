#include "engine/renderer/terrain.h"

#include <cmath>
#include <iostream>

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include "game/world/map_manager.h"
#include "engine/renderer/Camera.h"

namespace {
    float getNoise(float x, float z) {
        return 0.0f;
    }
}

void TerrainRenderer::Initialize() {
    if (initialized) {
        return;
    }

    shader.Load("assets/shaders/terrain_procedural.vert", "assets/shaders/terrain_procedural.frag");
    skyShader.Load("assets/shaders/sky.vert", "assets/shaders/sky.frag");

    glGenVertexArrays(1, &skyVAO);

    RebuildMesh();
    RebuildMountainMesh();
    initialized = true;
}

void TerrainRenderer::Update(const MapManager& mapManager) {
    if (!initialized) {
        Initialize();
    }
}

std::vector<Triangle> TerrainRenderer::GetTriangles() const {
    return collisionTriangles;
}

void TerrainRenderer::Render(const Camera& camera, float aspectRatio) {
    if (!initialized || !mesh.IsValid()) {
        return;
    }

    const glm::mat4 projection = camera.GetProjectionMatrix(aspectRatio);
    const glm::mat4 view = camera.GetViewMatrix();
    const glm::mat4 model = glm::mat4(1.0f);
    
    // Assuming aspect ratio is width/height and hardcoding to standard 720p base if unknown,
    // or better, approximate screen size from aspect ratio if height = 720
    glm::vec2 resolution(aspectRatio * 720.0f, 720.0f);

    // 1. Draw Sky
    glDepthMask(GL_FALSE);
    skyShader.Bind();
    glm::mat4 invProj = glm::inverse(projection);
    glm::mat4 invViewRot = glm::inverse(glm::mat4(glm::mat3(view))); // rotation only
    skyShader.SetMat4("invProj", invProj);
    skyShader.SetMat4("invViewRot", invViewRot);
    skyShader.SetVec3("lightDir", glm::vec3(0.6f, 1.0f, 0.4f));
    skyShader.SetVec2("resolution", resolution);
    glBindVertexArray(skyVAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    skyShader.Unbind();
    glDepthMask(GL_TRUE);

    // 2. Draw Terrain and Mountains
    shader.Bind();
    shader.SetMat4("projection", projection);
    shader.SetMat4("view", view);
    shader.SetMat4("model", model);
    
    // terrain shader uniforms
    shader.SetVec3("lightDir", glm::vec3(0.6f, 1.0f, 0.4f));
    shader.SetVec3("viewPos", camera.GetPosition());
    // Since we don't have a loaded grass texture yet, set hasTexture to false
    shader.SetInt("hasTexture", 0);
    shader.SetVec2("resolution", resolution);

    shader.SetFloat("objectType", 0.0f); // Terrain
    mesh.Draw();
    
    shader.SetFloat("objectType", 3.0f); // Mountains
    mountainMesh.Draw();

    shader.Unbind();
}

void TerrainRenderer::RebuildMesh() {
    std::vector<MeshVertex> vertices;
    std::vector<unsigned int> indices;
    collisionTriangles.clear();

    const int GRID_SIZE = 256;
    const int VERTEX_COUNT = GRID_SIZE + 1; 
    const float SIZE = 256.0f;
    
    vertices.resize(VERTEX_COUNT * VERTEX_COUNT);

    // 1. Generate Vertices
    for(int z = 0; z < VERTEX_COUNT; z++) {
        for(int x = 0; x < VERTEX_COUNT; x++) {
            float worldX = ((float)x / GRID_SIZE) * SIZE - (SIZE / 2.0f);
            float worldZ = ((float)z / GRID_SIZE) * SIZE - (SIZE / 2.0f);
            float worldY = getNoise(worldX, worldZ);
            
            MeshVertex& v = vertices[z * VERTEX_COUNT + x];
            v.position = glm::vec3(worldX, worldY, worldZ);
            v.normal = glm::vec3(0.0f, 1.0f, 0.0f);
            v.uv = glm::vec2((float)x / GRID_SIZE * 32.0f, (float)z / GRID_SIZE * 32.0f);
            v.color = glm::vec3(1.0f); // Default color
        }
    }

    // 2. Generate Indices
    for(int z = 0; z < GRID_SIZE; z++) {
        for(int x = 0; x < GRID_SIZE; x++) {
            unsigned int topLeft = (z * VERTEX_COUNT) + x;
            unsigned int topRight = topLeft + 1;
            unsigned int bottomLeft = ((z + 1) * VERTEX_COUNT) + x;
            unsigned int bottomRight = bottomLeft + 1;
            
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);
            
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    // 3. Compute Smooth Normals
    for(int i = 0; i < vertices.size(); i++) {
        vertices[i].normal = glm::vec3(0.0f);
    }

    for(size_t i = 0; i < indices.size(); i += 3) {
        unsigned int i0 = indices[i];
        unsigned int i1 = indices[i+1];
        unsigned int i2 = indices[i+2];
        
        glm::vec3 v0 = vertices[i0].position;
        glm::vec3 v1 = vertices[i1].position;
        glm::vec3 v2 = vertices[i2].position;
        
        glm::vec3 normal = glm::cross(v1 - v0, v2 - v0);
        if (normal.y < 0.0f) {
            normal = -normal;
        }
        
        vertices[i0].normal += normal;
        vertices[i1].normal += normal;
        vertices[i2].normal += normal;

        // Build collision triangles
        Triangle t;
        t.a = v0; t.b = v1; t.c = v2;
        collisionTriangles.push_back(t);
    }

    for(int i = 0; i < vertices.size(); i++) {
        vertices[i].normal = glm::normalize(vertices[i].normal);
    }

    mesh.Create(vertices, indices);
}

void TerrainRenderer::RebuildMountainMesh() {
    std::vector<MeshVertex> vertices;
    std::vector<unsigned int> indices;

    const int N = 128;
    const float radius = 180.0f;
    const float baseHeight = -2.0f;
    
    for (int i = 0; i <= N; ++i) { 
        float angle = ((float)i / N) * 2.0f * glm::pi<float>();
        
        float hNoise = sin(angle * 5.0f) * 0.5f 
                     + sin(angle * 13.0f + 2.0f) * 0.35f 
                     + sin(angle * 27.0f + 4.0f) * 0.20f;
        float height = 65.0f + hNoise * 35.0f; 
        
        float rNoise = sin(angle * 7.0f + 1.0f) * 0.5f + sin(angle * 19.0f) * 0.25f;
        float currentRadius = radius + rNoise * 15.0f;
        
        float x = cos(angle) * currentRadius;
        float z = sin(angle) * currentRadius;
        
        MeshVertex vBottom;
        vBottom.position = glm::vec3(x, baseHeight, z);
        vBottom.normal = glm::vec3(0.0f); 
        vBottom.uv = glm::vec2((float)i / N * 128.0f, 0.0f); 
        vBottom.color = glm::vec3(1.0f);
        vertices.push_back(vBottom);
        
        float topRadius = currentRadius + 30.0f; 
        float topX = cos(angle) * topRadius;
        float topZ = sin(angle) * topRadius;
        
        MeshVertex vTop;
        vTop.position = glm::vec3(topX, height, topZ);
        vTop.normal = glm::vec3(0.0f);
        vTop.uv = glm::vec2((float)i / N * 128.0f, 8.0f); 
        vTop.color = glm::vec3(1.0f);
        vertices.push_back(vTop);
    }
    
    for (int i = 0; i < N; ++i) {
        unsigned int bottom0 = i * 2;
        unsigned int top0 = i * 2 + 1;
        unsigned int bottom1 = (i + 1) * 2;
        unsigned int top1 = (i + 1) * 2 + 1;
        
        indices.push_back(bottom0);
        indices.push_back(top1);
        indices.push_back(top0);
        
        indices.push_back(bottom0);
        indices.push_back(bottom1);
        indices.push_back(top1);
    }
    
    for (size_t i = 0; i < indices.size(); i += 3) {
        glm::vec3 v0 = vertices[indices[i]].position;
        glm::vec3 v1 = vertices[indices[i+1]].position;
        glm::vec3 v2 = vertices[indices[i+2]].position;
        
        glm::vec3 normal = glm::cross(v1 - v0, v2 - v0);
        // Ensure normals point INWARDS toward the terrain (center = 0,0,0)
        glm::vec3 centerDir = glm::vec3(-v0.x, 0.0f, -v0.z);
        if (glm::dot(normal, centerDir) < 0.0f) {
            normal = -normal; 
        }
        
        vertices[indices[i]].normal += normal;
        vertices[indices[i+1]].normal += normal;
        vertices[indices[i+2]].normal += normal;
    }
    
    for (size_t i = 0; i < vertices.size(); ++i) {
        if (glm::length(vertices[i].normal) > 0.0001f) {
            vertices[i].normal = glm::normalize(vertices[i].normal);
        } else {
            vertices[i].normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }
    }

    mountainMesh.Create(vertices, indices);
}