#include "Sara.h"

#include <algorithm>
#include <random>

#include "core/InputManager.h"
#include "renderer/Camera.h"

namespace {

float Random01() {
	static std::mt19937 rng(std::random_device{}());
	static std::uniform_real_distribution<float> dist(0.0f, 1.0f);
	return dist(rng);
}

float RandomRange(float minValue, float maxValue) {
	static std::mt19937 rng(std::random_device{}());
	std::uniform_real_distribution<float> dist(minValue, maxValue);
	return dist(rng);
}

void EmitEvent(const char* /*eventName*/) {
}

void EmitRedemptionChanged(float /*value*/) {
}

void PlayWhisperAt(const glm::vec3& /*pos*/) {
}

} // namespace

Sara::Sara() {
	name = "Sara";
	stats.detectionMultiplier = 0.8f;
}

void Sara::Update(float dt) {
	voiceWarningProb = 0.8f + (redemptionScore / 100.0f) * 0.18f;
	voiceWarningProb = std::clamp(voiceWarningProb, 0.0f, 1.0f);

	if (ghostStepActive) {
		ghostStepTimer -= dt;
		stats.detectionMultiplier = 0.0f;
		stats.noiseRadius = 0.0f;
		if (ghostStepTimer <= 0.0f) {
			ghostStepActive = false;
			ghostStepTimer = 0.0f;
			stats.detectionMultiplier = 0.8f;
			stats.noiseRadius = 3.0f;
		}
	}

	if (ghostStepCooldown > 0.0f) {
		ghostStepCooldown -= dt;
		if (ghostStepCooldown < 0.0f) {
			ghostStepCooldown = 0.0f;
		}
	}
}

void Sara::HandleInput(InputManager& input, Camera& camera) {
	(void)input;
	(void)camera;
}

void Sara::NotifyCreatureApproaching(int creatureId, glm::vec3 futurePos) {
	survivedCreatureEncounters[creatureId] = true;

	const float rand01 = Random01();
	if (rand01 < voiceWarningProb) {
		PlayWhisperAt(futurePos);
	} else {
		const glm::vec3 fakePos = futurePos + glm::vec3(RandomRange(-10.0f, 10.0f), 0.0f, RandomRange(-10.0f, 10.0f));
		PlayWhisperAt(fakePos);
	}
}

void Sara::OnAbilityActivate() {
	if (ghostStepCooldown > 0.0f) {
		return;
	}

	ghostStepActive = true;
	ghostStepTimer = 4.0f;
	ghostStepCooldown = 30.0f;
	EmitEvent("SARA_GHOST_STEP_START");
}

void Sara::AddRedemptionScore(float amount) {
	redemptionScore = std::clamp(redemptionScore + amount, 0.0f, 100.0f);
	EmitRedemptionChanged(redemptionScore);
}

std::string Sara::GetCharacterName() const {
	return "Sara";
}

PostFXState Sara::GetPostFXState() const {
	PostFXState state;
	state.uvRippleStrength = ghostStepActive ? 0.8f : 0.0f;
	state.desaturation = (stats.health < 30.0f) ? ((30.0f - stats.health) / 30.0f) * 0.5f : 0.0f;
	return state;
}

