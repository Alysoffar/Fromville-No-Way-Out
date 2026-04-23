#include "Victor.h"

#include <algorithm>

#include "core/InputManager.h"
#include "renderer/Camera.h"

namespace {

void EmitEventWithPosition(const char* /*eventName*/, const glm::vec3& /*position*/) {
}

void SetGlobalTimeScale(float /*scale*/) {
}

bool IsCtrlHeldGlobal() {
	return false;
}

} // namespace

Victor::Victor() {
	name = "Victor";
	stats.detectionMultiplier = 0.6f;
	stats.moveSpeed = 4.0f;
}

void Victor::Update(float dt) {
	if (memoryModeActive) {
		memoryModeTimer -= dt;
		if (memoryModeTimer <= 0.0f) {
			memoryModeActive = false;
			memoryModeTimer = 0.0f;
			memoryModeCooldown = 60.0f;
			SetGlobalTimeScale(1.0f);
		}
	}

	if (memoryModeCooldown > 0.0f) {
		memoryModeCooldown -= dt;
		if (memoryModeCooldown < 0.0f) {
			memoryModeCooldown = 0.0f;
		}
	}

	if (IsCtrlHeldGlobal()) {
		if (!breathHolding) {
			breathHolding = true;
			breathHoldTimer = 5.0f;
		}
	} else {
		breathHolding = false;
		breathHoldTimer = 0.0f;
		stats.detectionMultiplier = 0.6f;
	}

	if (breathHolding) {
		breathHoldTimer -= dt;
		stats.detectionMultiplier = 0.3f;
		if (breathHoldTimer <= 0.0f) {
			breathHolding = false;
			breathHoldTimer = 0.0f;
			stats.detectionMultiplier = 0.6f;
		}
	}
}

void Victor::HandleInput(InputManager& input, Camera& camera) {
	(void)input;
	(void)camera;
}

void Victor::OnAbilityActivate() {
	if (memoryModeCooldown > 0.0f || memoryModeActive) {
		return;
	}

	memoryModeActive = true;
	memoryModeTimer = 5.0f;
	SetGlobalTimeScale(0.2f);
	EmitEventWithPosition("VICTOR_MEMORY_MODE", position);
}

std::string Victor::GetCharacterName() const {
	return "Victor";
}

PostFXState Victor::GetPostFXState() const {
	PostFXState state;
	state.desaturation = memoryModeActive ? 0.8f : 0.0f;
	state.colorGrade = memoryModeActive ? glm::vec3(0.9f, 0.85f, 0.7f) : glm::vec3(1.0f);
	return state;
}

