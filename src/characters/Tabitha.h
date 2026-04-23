#pragma once

#include <vector>

#include "Character.h"

class InputManager;
class Camera;
class TunnelNetwork;

class Tabitha final : public Character {
public:
	Tabitha();

	TunnelNetwork* tunnelRef = nullptr;
	std::vector<int> revealedTunnelSections;
	bool isInTunnels = false;
	float listenCooldown = 0.0f;

	void Update(float dt) override;
	void HandleInput(InputManager& input, Camera& camera) override;
	void OnAbilityActivate() override;
	std::string GetCharacterName() const override;
	PostFXState GetPostFXState() const override;
};

