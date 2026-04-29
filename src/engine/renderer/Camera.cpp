#include "engine/renderer/Camera.h"

#include <algorithm>

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include "engine/core/InputManager.h"

Camera::Camera() = default;

glm::mat4 Camera::GetViewMatrix() const {
    // Look directly at the calculated forward direction
    return glm::lookAt(position, position + GetForward(), GetUp());
}

glm::mat4 Camera::GetProjectionMatrix(float aspect) const {
    return glm::perspective(glm::radians(fov), aspect, nearPlane, farPlane);
}

// Notice the signature now matches your .h file exactly
void Camera::Update(InputManager& input, float dt, const glm::vec3& playerPosition) {
    (void)dt; // Silences the "unused parameter" warning

    // 1. Process Mouse Movement (Look around)
    const glm::vec2 mouseDelta = input.GetMouseDelta();
    ProcessMouseMovement(mouseDelta.x * mouseSensitivity, -mouseDelta.y * mouseSensitivity);

    // 2. Determine exactly where we want the camera to focus (the player's upper body)
    glm::vec3 lookAtTarget = playerPosition + glm::vec3(0.0f, targetHeightOffset, 0.0f);

    // 3. Move the camera backwards from the player based on the forward vector
    position = lookAtTarget - (GetForward() * targetDistance);
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset) {
    yaw += xoffset;
    
    // Clamp the pitch so the camera doesn't flip upside down or go too far underground
    pitch = std::clamp(pitch + yoffset, -15.0f, 85.0f);
}

void Camera::SetRotation(float newYaw, float newPitch) {
    yaw = newYaw;
    pitch = std::clamp(newPitch, -89.0f, 89.0f);
}

void Camera::Reset(const glm::vec3& spawnPosition) {
    position = spawnPosition;
    yaw = -90.0f;
    pitch = 0.0f;
}

glm::vec3 Camera::GetPosition() const {
    return position;
}

glm::vec3 Camera::GetForward() const {
    glm::vec3 forward;
    forward.x = std::cos(glm::radians(yaw)) * std::cos(glm::radians(pitch));
    forward.y = std::sin(glm::radians(pitch));
    forward.z = std::sin(glm::radians(yaw)) * std::cos(glm::radians(pitch));
    return glm::normalize(forward);
}

glm::vec3 Camera::GetRight() const {
    return glm::normalize(glm::cross(GetForward(), worldUp));
}

glm::vec3 Camera::GetUp() const {
    return glm::normalize(glm::cross(GetRight(), GetForward()));
}