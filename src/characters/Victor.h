#pragma once

#include "Character.h"

class InputManager;
class Camera;

class Victor final : public Character {
public:
	Victor();

	bool memoryModeActive = false;
	float memoryModeTimer = 0.0f;
	float memoryModeCooldown = 0.0f;
	int talismansCarried = 3;
	int elderTalismansCarried = 0;
	bool breathHolding = false;
	float breathHoldTimer = 0.0f;

	void Update(float dt) override;
	void HandleInput(InputManager& input, Camera& camera) override;
	void OnAbilityActivate() override;
	std::string GetCharacterName() const override;
	PostFXState GetPostFXState() const override;
};

