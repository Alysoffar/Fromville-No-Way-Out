#pragma once

#include <glm/glm.hpp>

class InputManager;

class Camera {
public:
    Camera();

    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix(float aspect) const;

    // Follow the active character with mouse-controlled third-person orbit.
    void Update(InputManager& input, float dt, const glm::vec3& playerPosition);
    void ProcessMouseMovement(float xoffset, float yoffset);
    void SnapToTarget(const glm::vec3& playerPosition);

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
    
    float mouseSensitivity = 0.05f;
    float fov = 60.0f;
    float nearPlane = 0.1f;
    float farPlane = 10000.0f;

    // Follow-camera parameters
    float followYaw = -135.0f;
    float targetDistance = 7.5f;
    float targetHeightOffset = 1.8f;
    float followPitch = 12.0f;
};