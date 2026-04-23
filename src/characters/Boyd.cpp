#include "Boyd.h"

#include <algorithm>

#include "core/InputManager.h"
#include "renderer/Camera.h"

namespace {

bool IsNightTime() {
	return true;
}

float QueryNearestCreatureDistance(const glm::vec3& /*position*/) {
	return 1000.0f;
}

void SetCurseAuraActive(void* /*aura*/, bool /*active*/) {
}

void EmitEvent(const char* /*eventName*/) {
}

void EmitNpcInterrogatedEvent(void* /*npc*/) {
}

bool AutoAttackNearestCreature(const glm::vec3& /*position*/, float /*maxSeekDistance*/, float /*damage*/) {
	return false;
}

struct NPCPlaceholder {
	void ForceInterrogateDialogueState() {}
	void RevealKnowledgeFlags() {}
};

NPCPlaceholder* FindNearestNpc(const glm::vec3& /*position*/, float /*radius*/) {
	return nullptr;
}

} // namespace

Boyd::Boyd() {
	name = "Boyd";
}

void Boyd::Update(float dt) {
	if (isActivePlayer && IsNightTime()) {
		const float nearest = QueryNearestCreatureDistance(position);
		if (nearest < 15.0f) {
			curseMeter += 0.01f * dt * (1.0f + (15.0f - nearest) / 15.0f);
		} else {
			curseMeter -= 0.005f * dt;
		}
		curseMeter = std::clamp(curseMeter, 0.0f, 1.0f);
	}

	SetCurseAuraActive(curseAura, curseMeter > 0.8f);

	if (curseMeter >= 1.0f && !inRageState) {
		inRageState = true;
		rageTimer = 3.0f;
		if (!rageSpeedApplied) {
			stats.moveSpeed *= 1.3f;
			stats.runSpeed *= 1.3f;
			rageSpeedApplied = true;
		}
		EmitEvent("BOYD_RAGE_START");
	}

	if (inRageState) {
		rageTimer -= dt;
		(void)AutoAttackNearestCreature(position, 25.0f, 50.0f);

		if (rageTimer <= 0.0f) {
			inRageState = false;
			curseMeter = 0.5f;
			if (rageSpeedApplied) {
				stats.moveSpeed /= 1.3f;
				stats.runSpeed /= 1.3f;
				rageSpeedApplied = false;
			}
		}
	}
}

void Boyd::HandleInput(InputManager& input, Camera& camera) {
	(void)input;
	(void)camera;
}

void Boyd::OnAbilityActivate() {
	if (NPCPlaceholder* npc = FindNearestNpc(position, 3.0f)) {
		npc->ForceInterrogateDialogueState();
		npc->RevealKnowledgeFlags();
		EmitNpcInterrogatedEvent(npc);
	}
}

std::string Boyd::GetCharacterName() const {
	return "Boyd";
}

PostFXState Boyd::GetPostFXState() const {
	PostFXState state;
	state.redChromaticAberration = (curseMeter > 0.5f) ? (curseMeter - 0.5f) * 2.0f : 0.0f;
	state.desaturation = (stats.health < 30.0f) ? ((30.0f - stats.health) / 30.0f) * 0.6f : 0.0f;
	state.vignetteStrength = 0.3f + (1.0f - stats.health / std::max(stats.maxHealth, 0.0001f)) * 0.4f;
	return state;
}

