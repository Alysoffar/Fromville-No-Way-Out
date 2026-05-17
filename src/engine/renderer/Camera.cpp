#include "engine/renderer/Camera.h"

#include <cmath>
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
    float fov = (fovOverride >= 0.0f) ? fovOverride : defaultFov;
    return glm::perspective(glm::radians(fov), aspect, nearPlane, farPlane);
}

void Camera::Update(InputManager& input, float dt, const glm::vec3& playerPosition) {
    if (hasPositionOverride) {
        position = positionOverride;
        return; // skip normal position update during transition
    }

    (void)dt;
    const glm::vec2 mouseDelta = input.GetMouseDelta();
    ProcessMouseMovement(mouseDelta.x * mouseSensitivity, -mouseDelta.y * mouseSensitivity);

    position = playerPosition + glm::vec3(0.0f, eyeHeight, 0.0f);
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset) {
    yaw += xoffset;
    pitch = std::clamp(pitch + yoffset, -85.0f, 85.0f); // Allow fuller pitch up/down for first person
}

void Camera::SetRotation(float newYaw, float newPitch) {
    yaw = newYaw;
    pitch = std::clamp(newPitch, -89.0f, 89.0f);
}

void Camera::Reset(const glm::vec3& spawnPosition) {
    position = spawnPosition + glm::vec3(0.0f, eyeHeight, 0.0f);
    yaw = -90.0f;
    pitch = 0.0f;
}

void Camera::SnapToTarget(const glm::vec3& playerPosition) {
    position = playerPosition + glm::vec3(0.0f, eyeHeight, 0.0f);
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