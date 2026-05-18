#pragma once

#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "engine/renderer/Mesh.h"

class CollisionWorld;

class TreeRenderer {
public:
    TreeRenderer() = default;

    void init(CollisionWorld* collisionWorld = nullptr);
    void render(const glm::mat4& view, const glm::mat4& projection,
                const glm::vec3& cameraPos,
                const glm::vec3& lightDir, const glm::vec3& lightColor,
                const glm::vec3& ambient, float diffuseStrength,
                const glm::vec3& fogColor, float fogDensity);
    void cleanup();

private:
    struct TreeModel {
        Mesh mesh;
        GLuint instanceVBO = 0;
        int instanceCount = 0;
        std::vector<glm::mat4> instanceTransforms;
    };

    std::vector<TreeModel> treeModels;
    GLuint shaderProgram = 0;

    static constexpr int TREES_PER_MODEL = 1000;
};
