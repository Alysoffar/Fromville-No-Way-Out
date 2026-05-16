#include "game/entities/Door.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

void Door::Initialize(const std::string& doorName, 
                      const std::vector<MeshVertex>& vertices, 
                      const std::vector<unsigned int>& indices, 
                      glm::vec3 pos, 
                      float baseRot, 
                      float openOff,
                      glm::vec3 hingeLocalPos,
                      glm::vec3 interactPos) {
    name = doorName;
    position = pos;
    baseRotation = baseRot;
    hingeOffset = hingeLocalPos;
    interactPosition = interactPos;
    mesh.Create(vertices, indices);
}

void Door::Update(float dt) {
    // No longer animated
}

void Door::Interact() {
    // Interaction is now handled by the World via teleportation
}

glm::mat4 Door::GetModelMatrix() const {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
    model = glm::rotate(model, glm::radians(baseRotation), glm::vec3(0.0f, 1.0f, 0.0f));
    return model;
}

void Door::Render(Shader& shader, const glm::mat4& view, const glm::mat4& projection, 
                  const glm::vec3& lightDir, const glm::vec3& lightColor, 
                  const glm::vec3& ambient, float diffuseStrength, 
                  const glm::vec3& viewPos, const glm::vec3& fogColor, float fogDensity) {
    if (!mesh.IsValid()) return;

    shader.Bind();
    shader.SetMat4("projection", projection);
    shader.SetMat4("view", view);
    shader.SetMat4("model", GetModelMatrix());
    
    shader.SetVec3("uLightDir", lightDir);
    shader.SetVec3("uLightColor", lightColor);
    shader.SetVec3("uAmbient", ambient);
    shader.SetFloat("uDiffuseStrength", diffuseStrength);
    shader.SetVec3("uViewPos", viewPos);
    shader.SetVec3("uFogColor", fogColor);
    shader.SetFloat("uFogDensity", fogDensity);
    
    mesh.Draw();
}
