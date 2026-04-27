#pragma once

#include <glm/glm.hpp>

class InputManager;

class Camera {
public:
	Camera();

	glm::mat4 GetViewMatrix() const;
	glm::mat4 GetProjectionMatrix(float aspect) const;

	void Update(InputManager& input, float dt);
	void ProcessMouseMovement(float xoffset, float yoffset);
	void SetPosition(const glm::vec3& newPosition);
	void SetRotation(float newYaw, float newPitch);
	void Reset(const glm::vec3& spawnPosition);

	glm::vec3 GetPosition() const;
	glm::vec3 GetForward() const;
	glm::vec3 GetRight() const;
	glm::vec3 GetUp() const;

private:
	glm::vec3 position = glm::vec3(0.0f, 1.8f, 3.5f);
	glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
	float yaw = -90.0f;
	float pitch = 0.0f;
	float movementSpeed = 6.0f;
	float mouseSensitivity = 0.08f;
	float fov = 60.0f;
	float nearPlane = 0.1f;
	float farPlane = 500.0f;
};