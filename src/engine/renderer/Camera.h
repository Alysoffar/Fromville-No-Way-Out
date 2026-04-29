#pragma once

#include <glm/glm.hpp>

class InputManager;

class Camera {
public:
    Camera();

    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix(float aspect) const;

    // We now pass the player's position into the camera every frame!
    void Update(InputManager& input, float dt, const glm::vec3& playerPosition);
    void ProcessMouseMovement(float xoffset, float yoffset);

    glm::vec3 GetPosition() const;
    glm::vec3 GetForward() const;
    glm::vec3 GetRight() const;
    glm::vec3 GetUp() const;
    
    // Helper methods for rotation
    void SetRotation(float newYaw, float newPitch);
    void Reset(const glm::vec3& spawnPosition);

private:
    glm::vec3 position = glm::vec3(0.0f, 2.5f, 6.0f);
    glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    
    float yaw = -90.0f;
    float pitch = 0.0f;
    
    float mouseSensitivity = 0.08f;
    float fov = 60.0f;
    float nearPlane = 0.1f;
    float farPlane = 500.0f;

    // Follow-camera parameters
    float targetDistance = 6.0f;
    float targetHeightOffset = 1.8f;
};