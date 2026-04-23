#pragma once

#include <glm/glm.hpp>

class Camera;
class Character;
class InputManager;

class PlayerController {
public:
	PlayerController(Character* character, Camera* camera);
	void Update(float dt, InputManager& input);

private:
	Character* character;
	Camera* camera;
	float targetFacingAngle = 0.0f;
	float turnSpeed = 10.0f;
	bool isSprinting = false;
	bool isCrouching = false;
	glm::vec3 lastMovementDir = glm::vec3(0.0f, 0.0f, -1.0f);

	void HandleMovement(float dt, InputManager& input);
	void HandleActions(float dt, InputManager& input);
	void UpdateNoiseRadius();
	glm::vec3 GetCameraRelativeInput(InputManager& input);
};

