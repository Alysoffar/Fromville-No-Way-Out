#include "Camera.h"

#include <algorithm>

#include <glm/gtc/matrix_transform.hpp>

glm::mat4 Camera::GetViewMatrix() const {
	return glm::lookAt(position, target, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 Camera::GetProjectionMatrix(float aspect) const {
	return glm::perspective(glm::radians(fov), aspect, nearPlane, farPlane);
}

void Camera::Update(glm::vec3 characterPos, float dt) {
	if (isUnderground) {
		followDistance = 2.0f;
		heightOffset = 1.2f;
		fov = 70.0f;
	} else {
		followDistance = 3.5f;
		heightOffset = 1.8f;
		fov = 60.0f;
	}

	const glm::vec3 focusPoint = characterPos + glm::vec3(0.0f, isUnderground ? 1.0f : 1.45f, 0.0f);
	glm::vec3 offset(0.0f, heightOffset, -followDistance);
	glm::mat4 rotation(1.0f);
	rotation = glm::rotate(rotation, glm::radians(yaw), glm::vec3(0.0f, 1.0f, 0.0f));
	rotation = glm::rotate(rotation, glm::radians(pitch), glm::vec3(1.0f, 0.0f, 0.0f));
	glm::vec3 desiredPosition = focusPoint + glm::vec3(rotation * glm::vec4(offset, 1.0f));
	const float followFactor = std::clamp(20.0f * dt, 0.0f, 1.0f);
	position = glm::mix(position, desiredPosition, followFactor);
	target = glm::mix(target, focusPoint, followFactor);
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset) {
	yaw += xoffset;
	pitch += yoffset;
	pitch = std::clamp(pitch, -30.0f, 60.0f);
}

void Camera::DoCollisionPush(const glm::vec3& charPos, std::function<bool(glm::vec3, glm::vec3)> raycastFn) {
	if (!raycastFn) {
		return;
	}

	if (raycastFn(charPos, position)) {
		glm::vec3 direction = position - charPos;
		const float distance = glm::length(direction);
		if (distance > 0.0001f) {
			direction /= distance;
			position = charPos + direction * std::max(0.3f, distance - 0.3f);
		}
	}
}

void Camera::Reset(glm::vec3 characterPos) {
	target = characterPos + glm::vec3(0.0f, isUnderground ? 1.0f : 1.45f, 0.0f);
	glm::vec3 offset(0.0f, heightOffset, -followDistance);
	glm::mat4 rotation(1.0f);
	rotation = glm::rotate(rotation, glm::radians(yaw), glm::vec3(0.0f, 1.0f, 0.0f));
	rotation = glm::rotate(rotation, glm::radians(pitch), glm::vec3(1.0f, 0.0f, 0.0f));
	position = target + glm::vec3(rotation * glm::vec4(offset, 1.0f));
}

glm::vec3 Camera::GetForward() const {
	return glm::normalize(target - position);
}

glm::vec3 Camera::GetUp() const {
	return glm::vec3(0.0f, 1.0f, 0.0f);
}
