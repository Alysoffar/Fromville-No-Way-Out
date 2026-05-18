#include "engine/renderer/TreeRenderer.h"

#include <cstdlib>
#include <iostream>
#include <vector>
#include <fstream>
#include <filesystem>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "engine/resources/loader.h"
#include "engine/renderer/ShaderUtils.h"
#include "engine/physics/CollisionWorld.h"

void TreeRenderer::init(CollisionWorld* cw) {
    const char* modelPaths[] = {
        "assets/models/BirchTree_1.obj",
        "assets/models/BirchTree_2.obj",
        "assets/models/BirchTree_3.obj"
    };

    const uint32_t CACHE_VERSION = 1;
    std::string cachePath = "assets/cache/tree_instances.bin";
    bool loadedFromCache = false;
    std::vector<std::vector<glm::mat4>> allInstanceMatrices(3);

    if (std::filesystem::exists(cachePath)) {
        try {
            std::ifstream file(cachePath, std::ios::in | std::ios::binary);
            uint32_t version = 0;
            uint32_t numModels = 0;
            if (file.read(reinterpret_cast<char*>(&version), sizeof(version)) &&
                file.read(reinterpret_cast<char*>(&numModels), sizeof(numModels))) {
                if (version == CACHE_VERSION && numModels == 3) {
                    bool ok = true;
                    for (int m = 0; m < 3; ++m) {
                        uint32_t count = 0;
                        if (file.read(reinterpret_cast<char*>(&count), sizeof(count)) && count == TREES_PER_MODEL) {
                            allInstanceMatrices[m].resize(count);
                            if (!file.read(reinterpret_cast<char*>(allInstanceMatrices[m].data()), count * sizeof(glm::mat4))) {
                                ok = false;
                                break;
                            }
                        } else {
                            ok = false;
                            break;
                        }
                    }
                    if (ok) {
                        loadedFromCache = true;
                        std::cout << "[TreeRenderer] Loaded all tree instances from cache successfully.\n";
                    }
                }
            }
        } catch (const std::exception& e) {
            std::cout << "[TreeRenderer] Failed to read cache: " << e.what() << "\n";
        }
    }

    if (!loadedFromCache) {
        std::srand(12345); // deterministic seed
        for (int modelIdx = 0; modelIdx < 3; ++modelIdx) {
            allInstanceMatrices[modelIdx].resize(TREES_PER_MODEL);
            for (int i = 0; i < TREES_PER_MODEL; ++i) {
                float x, z;
                bool inHouseArea = true;
                while (inHouseArea) {
                    x = (static_cast<float>(std::rand()) / RAND_MAX) * 300.0f - 150.0f;
                    z = (static_cast<float>(std::rand()) / RAND_MAX) * 300.0f - 150.0f;

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
                    bool inColony = (x > 4.0f && x < 14.0f && z > 2.0f && z < 14.0f);
                    bool inPolice = glm::distance(glm::vec2(x, z), glm::vec2(-65.84f, 15.84f)) < 11.3f;

                    if (!inAnyHouse && !inDinner && !inColony && !inPolice) {
                        inHouseArea = false;
                    }
                }

                float scale = 0.8f + (static_cast<float>(std::rand()) / RAND_MAX) * 0.7f; // 0.8 to 1.5
                float rotation = (static_cast<float>(std::rand()) / RAND_MAX) * 360.0f;

                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(x, 0.0f, z));
                model = glm::rotate(model, glm::radians(rotation), glm::vec3(0.0f, 1.0f, 0.0f));
                model = glm::scale(model, glm::vec3(scale * 5.5f));

                allInstanceMatrices[modelIdx][i] = model;
            }
        }

        try {
            std::filesystem::create_directories("assets/cache");
            std::ofstream file(cachePath, std::ios::out | std::ios::binary);
            uint32_t numModels = 3;
            file.write(reinterpret_cast<const char*>(&CACHE_VERSION), sizeof(CACHE_VERSION));
            file.write(reinterpret_cast<const char*>(&numModels), sizeof(numModels));
            for (int m = 0; m < 3; ++m) {
                uint32_t count = TREES_PER_MODEL;
                file.write(reinterpret_cast<const char*>(&count), sizeof(count));
                file.write(reinterpret_cast<const char*>(allInstanceMatrices[m].data()), count * sizeof(glm::mat4));
            }
            std::cout << "[TreeRenderer] Saved procedurally generated tree instances to cache.\n";
        } catch (const std::exception& e) {
            std::cout << "[TreeRenderer] Failed to write cache: " << e.what() << "\n";
        }
    }

    for (int modelIdx = 0; modelIdx < 3; ++modelIdx) {
        std::vector<MeshVertex> vertices;
        std::vector<unsigned int> indices;

        if (!Loader::LoadOBJ(modelPaths[modelIdx], vertices, indices)) {
            std::cerr << "[TreeRenderer] Failed to load " << modelPaths[modelIdx] << "\n";
            continue;
        }

        float maxY = -1e9f;
        for (const auto& v : vertices) {
            if (v.position.y > maxY) maxY = v.position.y;
        }
        for (auto& v : vertices) {
            float r2 = v.position.x * v.position.x + v.position.z * v.position.z;
            // Radius < ~0.3 and height < 70% is trunk
            if (r2 < 0.09f && v.position.y < maxY * 0.7f) { 
                v.color = glm::vec3(0.85f, 0.85f, 0.85f);
            } else {
                // Add a vertical gradient to the leaves for fake ambient occlusion/depth
                float tint = 0.6f + 0.4f * (v.position.y / maxY);
                v.color = glm::vec3(0.2f * tint, 0.6f * tint, 0.15f * tint);
            }
        }

        TreeModel treeModel;
        treeModel.mesh.Create(vertices, indices);
        treeModel.instanceCount = TREES_PER_MODEL;

        std::vector<glm::mat4> instanceMatrices = allInstanceMatrices[modelIdx];
        treeModel.instanceTransforms = instanceMatrices;

        if (cw) {
            // Pre-extract only the trunk triangles (radius < 0.3, height < 70%)
            std::vector<Triangle> baseTris;
            for (size_t i = 0; i < indices.size(); i += 3) {
                auto& v1 = vertices[indices[i]];
                auto& v2 = vertices[indices[i+1]];
                auto& v3 = vertices[indices[i+2]];
                
                float r1 = v1.position.x*v1.position.x + v1.position.z*v1.position.z;
                float r2 = v2.position.x*v2.position.x + v2.position.z*v2.position.z;
                float r3 = v3.position.x*v3.position.x + v3.position.z*v3.position.z;
                
                if (r1 < 0.15f && r2 < 0.15f && r3 < 0.15f &&
                    v1.position.y < maxY * 0.7f && v2.position.y < maxY * 0.7f && v3.position.y < maxY * 0.7f) {
                    Triangle tri;
                    tri.a = v1.position;
                    tri.b = v2.position;
                    tri.c = v3.position;
                    baseTris.push_back(tri);
                }
            }

            std::vector<Triangle> worldTris;
            worldTris.reserve(instanceMatrices.size() * baseTris.size());
            for (const auto& mat : instanceMatrices) {
                for (const auto& tri : baseTris) {
                    Triangle wt;
                    wt.a = glm::vec3(mat * glm::vec4(tri.a, 1.0f));
                    wt.b = glm::vec3(mat * glm::vec4(tri.b, 1.0f));
                    wt.c = glm::vec3(mat * glm::vec4(tri.c, 1.0f));
                    worldTris.push_back(wt);
                }
            }
            cw->AddTriangles(worldTris);
        }

        glGenBuffers(1, &treeModel.instanceVBO);
        glBindBuffer(GL_ARRAY_BUFFER, treeModel.instanceVBO);
        glBufferData(GL_ARRAY_BUFFER, instanceMatrices.size() * sizeof(glm::mat4), instanceMatrices.data(), GL_DYNAMIC_DRAW);

        treeModel.mesh.Bind();
        for (int i = 0; i < 4; i++) {
            glEnableVertexAttribArray(4 + i);
            glVertexAttribPointer(4 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4) * i));
            glVertexAttribDivisor(4 + i, 1);
        }
        glBindVertexArray(0);

        treeModels.push_back(std::move(treeModel));
    }

    // If we attached triangles to the collision world, rebuild its BVH so collisions include trees
    if (cw) {
        cw->BuildBVH();
    }

    shaderProgram = ShaderUtils::loadShaderProgram("shaders/tree.vert", "shaders/tree.frag");
    if (shaderProgram == 0) {
        std::cerr << "[TreeRenderer] Failed to load tree shaders!\n";
    }

    std::cout << "[TreeRenderer] Initialized with " << (treeModels.size() * TREES_PER_MODEL) << " trees.\n";
}

void TreeRenderer::render(const glm::mat4& view, const glm::mat4& projection,
                          const glm::vec3& cameraPos,
                          const glm::vec3& lightDir, const glm::vec3& lightColor,
                          const glm::vec3& ambient, float diffuseStrength,
                          const glm::vec3& fogColor, float fogDensity) {
    if (shaderProgram == 0) return;

    glUseProgram(shaderProgram);

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uView"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uProjection"), 1, GL_FALSE, glm::value_ptr(projection));

    glUniform3fv(glGetUniformLocation(shaderProgram, "uLightDir"), 1, glm::value_ptr(lightDir));
    glUniform3fv(glGetUniformLocation(shaderProgram, "uLightColor"), 1, glm::value_ptr(lightColor));
    glUniform3fv(glGetUniformLocation(shaderProgram, "uAmbient"), 1, glm::value_ptr(ambient));
    glUniform1f(glGetUniformLocation(shaderProgram, "uDiffuseStrength"), diffuseStrength);
    glUniform3fv(glGetUniformLocation(shaderProgram, "uViewPos"), 1, glm::value_ptr(cameraPos));
    glUniform3fv(glGetUniformLocation(shaderProgram, "uFogColor"), 1, glm::value_ptr(fogColor));
    glUniform1f(glGetUniformLocation(shaderProgram, "uFogDensity"), fogDensity);

    static constexpr float TREE_RENDER_RADIUS = 60.0f;
    static constexpr float TREE_RENDER_RADIUS_SQ = TREE_RENDER_RADIUS * TREE_RENDER_RADIUS;

    for (auto& tm : treeModels) {
        if (tm.mesh.IsValid()) {
            std::vector<glm::mat4> visibleTransforms;
            visibleTransforms.reserve(tm.instanceTransforms.size());

            for (const auto& transform : tm.instanceTransforms) {
                // The translation is in the 4th column of the mat4
                float x = transform[3][0];
                float z = transform[3][2];
                float dx = x - cameraPos.x;
                float dz = z - cameraPos.z;
                float distSq = dx * dx + dz * dz;

                if (distSq <= TREE_RENDER_RADIUS_SQ) {
                    visibleTransforms.push_back(transform);
                }
            }

            if (!visibleTransforms.empty()) {
                glBindBuffer(GL_ARRAY_BUFFER, tm.instanceVBO);
                glBufferData(GL_ARRAY_BUFFER, visibleTransforms.size() * sizeof(glm::mat4), visibleTransforms.data(), GL_DYNAMIC_DRAW);

                tm.mesh.Bind();
                glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLsizei>(tm.mesh.GetIndexCount()), GL_UNSIGNED_INT, nullptr, static_cast<GLsizei>(visibleTransforms.size()));
                glBindVertexArray(0);
            }
        }
    }

    glUseProgram(0);
}

void TreeRenderer::cleanup() {
    for (auto& tm : treeModels) {
        if (tm.instanceVBO != 0) {
            glDeleteBuffers(1, &tm.instanceVBO);
            tm.instanceVBO = 0;
        }
    }
    treeModels.clear();

    if (shaderProgram != 0) {
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
    }
}
