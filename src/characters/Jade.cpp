#include "Jade.h"

#include <algorithm>

#include "core/InputManager.h"
#include "renderer/Camera.h"

namespace {
void EmitEvent(const char* /*eventName*/) {
}
} // namespace

Jade::Jade() {
	name = "Jade";
	stats.detectionMultiplier = 0.85f;
}

void Jade::Update(float dt) {
	if (symbolSightActive) {
		symbolSightTimer -= dt;
		if (symbolSightTimer <= 0.0f) {
			symbolSightActive = false;
			symbolSightTimer = 0.0f;
		}
	}

	visionTimer += dt;
	if (visionTimer >= visionCooldown) {
		visionTimer = 0.0f;
		EmitEvent("JADE_VISION_TRIGGER");
	}
}

void Jade::HandleInput(InputManager& input, Camera& camera) {
	(void)input;
	(void)camera;
}

void Jade::OnAbilityActivate() {
	symbolSightActive = true;
	symbolSightTimer = 8.0f;
	EmitEvent("SYMBOL_SIGHT_ACTIVATED");
}

std::string Jade::GetCharacterName() const {
	return "Jade";
}

PostFXState Jade::GetPostFXState() const {
	PostFXState state;
	state.symbolSightStrength = symbolSightActive ? 1.0f : 0.0f;
	state.desaturation = symbolSightActive ? 0.6f : 0.0f;
	state.colorGrade = symbolSightActive ? glm::vec3(1.1f, 0.95f, 0.7f) : glm::vec3(1.0f);
	return state;
}

