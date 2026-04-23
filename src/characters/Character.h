#pragma once

#include <string>

#include <glm/glm.hpp>

#include "renderer/Model.h"
#include "renderer/PostFXState.h"
#include "renderer/SkeletalAnimator.h"

class Camera;
class InputManager;

struct CharacterStats {
	float health = 100.0f;
	float maxHealth = 100.0f;
	float stamina = 100.0f;
	float maxStamina = 100.0f;
	float staminaRegenRate = 15.0f;
	float moveSpeed = 4.5f;
	float runSpeed = 8.0f;
	float crouchSpeed = 2.0f;
	float noiseRadius = 3.0f;
	float detectionMultiplier = 1.0f;
};

enum class CharacterState {
	IDLE,
	WALKING,
	RUNNING,
	CROUCHING,
	INTERACTING,
	USING_ABILITY,
	HURT,
	DEAD,
	IDLE_OFFSCREEN
};

class Character {
public:
	std::string name;
	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 velocity = glm::vec3(0.0f);
	float facingAngle = 0.0f;
	CharacterStats stats;
	CharacterState state = CharacterState::IDLE;
	Model* model = nullptr;
	SkeletalAnimator* animator = nullptr;
	bool isActivePlayer = false;

	virtual ~Character() = default;
	virtual void Update(float dt) = 0;
	virtual void HandleInput(InputManager& input, Camera& camera) = 0;
	virtual void OnAbilityActivate() = 0;
	virtual std::string GetCharacterName() const = 0;
	virtual PostFXState GetPostFXState() const;
	virtual void OnBecomeActive();
	virtual void OnBecomeInactive();

	void Move(glm::vec3 dir, float speed, float dt);
	void TakeDamage(float amount);
	void Heal(float amount);
	void UseStamina(float amount);
	bool IsAlive() const { return stats.health > 0.0f; }
	bool IsDead() const { return state == CharacterState::DEAD; }
	float GetNoiseLevelThisFrame() const { return currentNoiseRadius; }
	void SetNoiseLevelThisFrame(float radius) { currentNoiseRadius = radius; }

protected:
	float currentNoiseRadius = 0.0f;
	void UpdateAnimationState();
	void SimulateOffscreen(float dt);
};

