#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "engine/renderer/Mesh.h"
#include "engine/renderer/Shader.h"

class Door {
public:
    Door() = default;
    
    void Initialize(const std::string& name, 
                    const std::vector<MeshVertex>& vertices, 
                    const std::vector<unsigned int>& indices, 
                    glm::vec3 position, 
                    float baseRotation, 
                    float openOffsetAngle,
                    glm::vec3 hingeLocalPos,
                    glm::vec3 interactPos);
    
    void Update(float dt);
    void Interact();
    void Render(Shader& shader, const glm::mat4& view, const glm::mat4& projection, 
                const glm::vec3& lightDir, const glm::vec3& lightColor, 
                const glm::vec3& ambient, float diffuseStrength, 
                const glm::vec3& viewPos, const glm::vec3& fogColor, float fogDensity);

    glm::vec3 GetPosition() const { return interactPosition; }
    glm::vec3 GetBuildingPosition() const { return position; }
    const std::string& GetName() const { return name; }
    
    // Position to teleport the player to when entering
    glm::vec3 GetInsidePosition() const { return insidePosition; }
    void SetInsidePosition(glm::vec3 pos) { insidePosition = pos; }
    
    glm::vec3 GetOutsidePosition() const { return outsidePosition; }
    void SetOutsidePosition(glm::vec3 pos) { outsidePosition = pos; }

private:
    std::string name;
    Mesh mesh;
    glm::vec3 position; // Building position
    float baseRotation; // Building rotation
    glm::vec3 hingeOffset;
    glm::vec3 insidePosition;
    glm::vec3 outsidePosition;
    glm::vec3 interactPosition;

    glm::mat4 GetModelMatrix() const;
};

