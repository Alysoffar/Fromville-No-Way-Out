#pragma once

#include <functional>

#include <glm/glm.hpp>

class Camera {
public:
	glm::vec3 position = glm::vec3(0.0f, 1.8f, 3.5f);
	glm::vec3 target = glm::vec3(0.0f);
	float yaw = -90.0f;
	float pitch = 15.0f;
	float followDistance = 3.5f;
	float heightOffset = 1.8f;
	float fov = 60.0f;
	float nearPlane = 0.1f;
	float farPlane = 500.0f;
	bool isUnderground = false;

	glm::mat4 GetViewMatrix() const;
	glm::mat4 GetProjectionMatrix(float aspect) const;
	void Update(glm::vec3 characterPos, float dt);
	void ProcessMouseMovement(float xoffset, float yoffset);
	void DoCollisionPush(const glm::vec3& charPos, std::function<bool(glm::vec3, glm::vec3)> raycastFn);
	void Reset(glm::vec3 characterPos);
	
	glm::vec3 GetForward() const;
	glm::vec3 GetUp() const;
};