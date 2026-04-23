#pragma once

#include "Character.h"

class InputManager;
class Camera;
class ParticleSystem;

class Boyd final : public Character {
public:
	Boyd();

	float curseMeter = 0.0f;
	bool inRageState = false;
	float rageTimer = 0.0f;
	int revolverAmmo = 6;
	int reserveAmmo = 18;
	bool hasTunnelMapPartial = true;
	ParticleSystem* curseAura = nullptr;

	void Update(float dt) override;
	void HandleInput(InputManager& input, Camera& camera) override;
	void OnAbilityActivate() override;
	std::string GetCharacterName() const override;
	PostFXState GetPostFXState() const override;

private:
	bool rageSpeedApplied = false;
};

