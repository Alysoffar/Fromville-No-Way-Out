#include "Tabitha.h"

#include <algorithm>

#include "core/InputManager.h"
#include "renderer/Camera.h"

namespace {

bool IsDaytime() {
	return false;
}

void EmitEvent(const char* /*eventName*/) {
}

void EmitTunnelSectionRevealed(int /*sectionId*/) {
}

int QueryTunnelSectionAt(void* /*tunnelRef*/, const glm::vec3& /*pos*/) {
	return -1;
}

bool ForwardRayHitsWall(const glm::vec3& /*origin*/, float /*distance*/, float /*facingAngle*/) {
	return false;
}

bool HasCreaturesBehindWall(const glm::vec3& /*origin*/, float /*radius*/) {
	return false;
}

void PlaySoundAtPlayer(const char* /*soundPath*/, const glm::vec3& /*pos*/) {
}

} // namespace

Tabitha::Tabitha() {
	name = "Tabitha";
}

void Tabitha::Update(float dt) {
	const bool underground = (position.y < -2.0f);
	if (underground != isInTunnels) {
		isInTunnels = underground;
		if (isInTunnels && !IsDaytime()) {
			TakeDamage(0.0f);
			EmitEvent("TABITHA_NIGHT_TUNNEL_WARNING");
		}
	}

	if (isInTunnels && tunnelRef != nullptr) {
		const int section = QueryTunnelSectionAt(tunnelRef, position);
		if (section >= 0) {
			const bool known = std::find(revealedTunnelSections.begin(), revealedTunnelSections.end(), section) != revealedTunnelSections.end();
			if (!known) {
				revealedTunnelSections.push_back(section);
				EmitTunnelSectionRevealed(section);
			}
		}
	}

	if (listenCooldown > 0.0f) {
		listenCooldown -= dt;
		if (listenCooldown < 0.0f) {
			listenCooldown = 0.0f;
		}
	}
}

void Tabitha::HandleInput(InputManager& input, Camera& camera) {
	(void)input;
	(void)camera;
}

void Tabitha::OnAbilityActivate() {
	if (listenCooldown > 0.0f) {
		return;
	}

	listenCooldown = 4.0f;
	const bool hitsWall = ForwardRayHitsWall(position, 1.5f, facingAngle);
	if (!hitsWall) {
		return;
	}

	if (HasCreaturesBehindWall(position, 5.0f)) {
		PlaySoundAtPlayer("wall_creature_nearby.ogg", position);
	} else {
		PlaySoundAtPlayer("wall_clear.ogg", position);
	}
}

std::string Tabitha::GetCharacterName() const {
	return "Tabitha";
}

PostFXState Tabitha::GetPostFXState() const {
	PostFXState state;
	state.isUnderground = isInTunnels;
	if (isInTunnels) {
		state.colorGrade = glm::vec3(0.7f, 0.75f, 1.0f);
	}
	return state;
}

