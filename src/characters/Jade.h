#pragma once

#include <vector>

#include "Character.h"

class InputManager;
class Camera;

class Jade final : public Character {
public:
	Jade();

	bool symbolSightActive = false;
	float symbolSightTimer = 0.0f;
	float visionCooldown = 300.0f;
	float visionTimer = 0.0f;
	std::vector<int> decodedGlyphs;

	void Update(float dt) override;
	void HandleInput(InputManager& input, Camera& camera) override;
	void OnAbilityActivate() override;
	std::string GetCharacterName() const override;
	PostFXState GetPostFXState() const override;
};

