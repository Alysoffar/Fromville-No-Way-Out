#pragma once

#include <unordered_map>

#include "Character.h"

class InputManager;
class Camera;

class Sara final : public Character {
public:
	Sara();

	float redemptionScore = 0.0f;
	bool ghostStepActive = false;
	float ghostStepTimer = 0.0f;
	float ghostStepCooldown = 0.0f;
	float voiceWarningProb = 0.8f;
	std::unordered_map<int, bool> survivedCreatureEncounters;

	void Update(float dt) override;
	void HandleInput(InputManager& input, Camera& camera) override;
	void OnAbilityActivate() override;
	std::string GetCharacterName() const override;
	PostFXState GetPostFXState() const override;

	void NotifyCreatureApproaching(int creatureId, glm::vec3 futurePos);
	void AddRedemptionScore(float amount);
};

