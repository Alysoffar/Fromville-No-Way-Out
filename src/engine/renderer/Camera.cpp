#include "engine/renderer/Camera.h"

#include <algorithm>

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include "engine/core/InputManager.h"

Camera::Camera() = default;

glm::mat4 Camera::GetViewMatrix() const {
	return glm::lookAt(position, position + GetForward(), GetUp());
}

glm::mat4 Camera::GetProjectionMatrix(float aspect) const {
	return glm::perspective(glm::radians(fov), aspect, nearPlane, farPlane);
}

void Camera::Update(InputManager& input, float dt) {
	const glm::vec3 forward = GetForward();
	const glm::vec3 right = GetRight();

	glm::vec3 movement(0.0f);
	if (input.IsKeyDown(GLFW_KEY_W) || input.IsKeyDown(GLFW_KEY_UP)) {
		movement += forward;
	}
	if (input.IsKeyDown(GLFW_KEY_S) || input.IsKeyDown(GLFW_KEY_DOWN)) {
		movement -= forward;
	}
	if (input.IsKeyDown(GLFW_KEY_D) || input.IsKeyDown(GLFW_KEY_RIGHT)) {
		movement += right;
	}
	if (input.IsKeyDown(GLFW_KEY_A) || input.IsKeyDown(GLFW_KEY_LEFT)) {
		movement -= right;
	}
	if (input.IsKeyDown(GLFW_KEY_SPACE)) {
		movement += worldUp;
	}
	if (input.IsKeyDown(GLFW_KEY_LEFT_CONTROL)) {
		movement -= worldUp;
	}

	if (glm::length(movement) > 0.0001f) {
		position += glm::normalize(movement) * movementSpeed * dt;
	}

	const glm::vec2 mouseDelta = input.GetMouseDelta();
	ProcessMouseMovement(mouseDelta.x * mouseSensitivity, -mouseDelta.y * mouseSensitivity);
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset) {
	yaw += xoffset;
	pitch = std::clamp(pitch + yoffset, -89.0f, 89.0f);
}

void Camera::SetPosition(const glm::vec3& newPosition) {
	position = newPosition;
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