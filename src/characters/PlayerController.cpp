#include "PlayerController.h"

#include <algorithm>

#include <glm/gtc/constants.hpp>

#include "Character.h"
#include "core/InputManager.h"
#include "renderer/Camera.h"

namespace {
struct Interactable {
	void Interact() {}
};

Interactable* FindNearestInteractable(const glm::vec3& /*position*/, float /*radius*/) {
	return nullptr;
}

void EmitCharacterSwitchRequestedEvent() {
	// Placeholder until EventBus event types are defined.
}
} // namespace

PlayerController::PlayerController(Character* character, Camera* camera)
	: character(character), camera(camera) {}

void PlayerController::Update(float dt, InputManager& input) {
	if (!character || !camera || !character->IsAlive()) {
		return;
	}

	HandleActions(dt, input);
	HandleMovement(dt, input);
	UpdateNoiseRadius();
}

glm::vec3 PlayerController::GetCameraRelativeInput(InputManager& input) {
	float forwardInput = 0.0f;
	float rightInput = 0.0f;

	if (input.IsKeyDown(GLFW_KEY_W)) forwardInput += 1.0f;
	if (input.IsKeyDown(GLFW_KEY_S)) forwardInput -= 1.0f;
	if (input.IsKeyDown(GLFW_KEY_D)) rightInput += 1.0f;
	if (input.IsKeyDown(GLFW_KEY_A)) rightInput -= 1.0f;

	glm::vec3 rawInput(rightInput, 0.0f, forwardInput);
	if (glm::length(rawInput) <= 0.0001f) {
		return glm::vec3(0.0f);
	}

	const float yawRad = glm::radians(camera->yaw);
	const glm::vec3 camForward(glm::sin(yawRad), 0.0f, -glm::cos(yawRad));
	const glm::vec3 camRight(glm::cos(yawRad), 0.0f, glm::sin(yawRad));

	glm::vec3 moveDir = camForward * forwardInput + camRight * rightInput;
	if (glm::length(moveDir) > 0.0001f) {
		moveDir = glm::normalize(moveDir);
	}
	return moveDir;
}

void PlayerController::HandleMovement(float dt, InputManager& input) {
	glm::vec3 moveDir = GetCameraRelativeInput(input);

	float speed = character->stats.moveSpeed;
	if (isCrouching) {
		speed = character->stats.crouchSpeed;
		character->state = CharacterState::CROUCHING;
	} else if (isSprinting && character->stats.stamina > 0.0f) {
		speed = character->stats.runSpeed;
		character->UseStamina(20.0f * dt);
		character->state = CharacterState::RUNNING;
	} else {
		character->state = CharacterState::WALKING;
	}

	if (glm::length(moveDir) <= 0.0001f) {
		if (character->stats.stamina < character->stats.maxStamina) {
			character->stats.stamina = std::min(
				character->stats.maxStamina,
				character->stats.stamina + character->stats.staminaRegenRate * dt);
		}
		character->state = isCrouching ? CharacterState::CROUCHING : CharacterState::IDLE;
	} else {
		lastMovementDir = moveDir;
		targetFacingAngle = std::atan2(moveDir.x, -moveDir.z);
		const float blend = std::clamp(turnSpeed * dt, 0.0f, 1.0f);
		character->facingAngle = glm::mix(character->facingAngle, targetFacingAngle, blend);
	}

	character->velocity.y -= 9.8f * dt;
	character->Move(moveDir, speed, dt);

	switch (character->state) {
	case CharacterState::RUNNING:
	case CharacterState::CROUCHING:
	case CharacterState::WALKING:
		break;
	default:
		character->state = CharacterState::IDLE;
		break;
	}
}

void PlayerController::HandleActions(float /*dt*/, InputManager& input) {
	if (input.IsKeyPressed(GLFW_KEY_E)) {
		if (Interactable* interactable = FindNearestInteractable(character->position, 2.0f)) {
			character->state = CharacterState::INTERACTING;
			interactable->Interact();
		}
	}

	if (input.IsKeyPressed(GLFW_KEY_Q)) {
		character->state = CharacterState::USING_ABILITY;
		character->OnAbilityActivate();
	}

	if (input.IsKeyPressed(GLFW_KEY_TAB)) {
		EmitCharacterSwitchRequestedEvent();
	}

	if (input.IsKeyPressed(GLFW_KEY_C)) {
		isCrouching = !isCrouching;
		if (isCrouching) {
			isSprinting = false;
		}
	}

	isSprinting = !isCrouching && input.IsKeyDown(GLFW_KEY_LEFT_SHIFT);
}

void PlayerController::UpdateNoiseRadius() {
	if (!character) {
		return;
	}

	if (character->state == CharacterState::RUNNING) {
		character->SetNoiseLevelThisFrame(8.0f);
	} else if (character->state == CharacterState::CROUCHING) {
		character->SetNoiseLevelThisFrame(1.0f);
	} else if (character->state == CharacterState::WALKING) {
		character->SetNoiseLevelThisFrame(3.0f);
	} else {
		character->SetNoiseLevelThisFrame(0.0f);
	}
}

