#include "CreatureAI.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include <glm/gtc/constants.hpp>

#include "NavMesh.h"
#include "characters/Character.h"

namespace {

bool IsNight(const DayNightCycle* /*dayNight*/) {
	return true;
}

float SecondsUntilSunrise(const DayNightCycle* /*dayNight*/) {
	return 120.0f;
}

bool RayBlockedByWorld(const glm::vec3& /*from*/, const glm::vec3& /*to*/) {
	return false;
}

bool IsNearWindowOrDoorWithoutTalisman(const glm::vec3& /*pos*/, float /*radius*/) {
	return false;
}

bool IsNpcIndoor(const NPC* /*npc*/) {
	return true;
}

bool NpcOpenedDoorOrWindow(const NPC* /*npc*/) {
	return false;
}

void PlayCreatureWhisperAudio(const glm::vec3& /*atPos*/) {
}

void DealDamageToPlayer(Character* player, float amount) {
	if (player) {
		player->TakeDamage(amount);
	}
}

glm::vec3 PlayerForward(const Character* player) {
	if (!player) {
		return glm::vec3(0.0f, 0.0f, 1.0f);
	}
	return glm::vec3(std::sin(player->facingAngle), 0.0f, std::cos(player->facingAngle));
}

Character* FindNearestPlayer(const Creature& c, std::vector<Character*>& players) {
	Character* best = nullptr;
	float bestDist2 = std::numeric_limits<float>::max();
	for (Character* p : players) {
		if (!p || !p->IsAlive()) {
			continue;
		}
		const glm::vec3 delta = p->position - c.position;
		const float d2 = glm::dot(delta, delta);
		if (d2 < bestDist2) {
			bestDist2 = d2;
			best = p;
		}
	}
	return best;
}

} // namespace

CreatureAI::CreatureAI(NavMesh* navMeshPtr, DayNightCycle* dayNightPtr)
	: navMesh(navMeshPtr), dayNight(dayNightPtr) {}

void CreatureAI::SpawnCreature(glm::vec3 position, std::vector<glm::vec3> patrolPath) {
	Creature c;
	c.id = static_cast<int>(creatures.size());
	c.position = position;
	c.lastKnownPlayerPos = position;
	c.patrolPath = std::move(patrolPath);
	c.state = IsNight(dayNight) ? CreatureState::PATROL : CreatureState::DORMANT;
	creatures.push_back(std::move(c));
	Creature& newCreature = creatures.back();

	// Problem 1: Load Models and Animators for the zombie
	try {
		newCreature.monsterModel = new Model("assets/models/zombie/Idle.dae");
		newCreature.monsterAnimator = new SkeletalAnimator(newCreature.monsterModel);
		
		// Load animations and log durations
		newCreature.monsterAnimator->LoadAnimationFromFile(Anim::IDLE, "assets/models/zombie/Idle.dae");
		newCreature.monsterAnimator->LoadAnimationFromFile(Anim::WALK, "assets/models/zombie/Walking.dae");
		newCreature.monsterAnimator->LoadAnimationFromFile(Anim::RUN, "assets/models/zombie/Walking.dae"); // Fallback
		newCreature.monsterAnimator->LoadAnimationFromFile(Anim::ATTACK, "assets/models/zombie/Idle.dae"); // Fallback
		
		std::printf("[spawn] Creature %d spawned.\n", newCreature.id);
		std::printf("  - idle: %.2f ticks\n", newCreature.monsterAnimator->GetAnimationDuration(Anim::IDLE));
		std::printf("  - walk: %.2f ticks\n", newCreature.monsterAnimator->GetAnimationDuration(Anim::WALK));
		std::printf("  - run: %.2f ticks\n", newCreature.monsterAnimator->GetAnimationDuration(Anim::RUN));
		std::printf("  - attack: %.2f ticks\n", newCreature.monsterAnimator->GetAnimationDuration(Anim::ATTACK));
	} catch (const std::exception& e) {
		std::printf("[spawn] Failed to load zombie model for creature %d: %s\n", newCreature.id, e.what());
	}
}

void CreatureAI::Update(float dt, std::vector<Character*>& players, std::vector<NPC*>& npcs) {
	for (Creature& c : creatures) {
		switch (c.state) {
		case CreatureState::DORMANT:
			UpdateDormant(c, dt);
			break;
		case CreatureState::PATROL:
			UpdatePatrol(c, dt, players);
			break;
		case CreatureState::SEARCH:
			UpdateSearch(c, dt, players);
			break;
		case CreatureState::HUNT:
			UpdateHunt(c, dt, players);
			break;
		case CreatureState::WHISPER:
			UpdateWhisper(c, dt, npcs);
			break;
		case CreatureState::RETREAT:
			UpdateRetreat(c, dt);
			break;
		}

		// Problem 9: Attack animation timer return
		if (c.attackAnimTimer > 0.0f) {
			c.attackAnimTimer -= dt;
			if (c.attackAnimTimer <= 0.0f) {
				c.monsterAnimator->BlendTo(Anim::RUN, 0.15f);
			}
		}

		// Problem 1: Update and Upload Animator
		if (c.monsterAnimator) {
			c.monsterAnimator->Update(dt);
			c.monsterAnimator->UploadToUBO();
		}
	}
}

void CreatureAI::DrawAll(Renderer& renderer) {
	for (const Creature& c : creatures) {
		if (c.monsterModel) {
			// Submit to renderer... assuming we have a way to set the model matrix and shader
			// For now, we'll need a shader and the renderer to handle it.
			// Actually, let's just make sure they are updated. 
			// In this project, the actual drawing usually happens in the main loop or Game::Render
			// by accessing the creatures list.
		}
	}
}

void CreatureAI::NotifyPlayerAction(int playerId, std::string action, glm::vec3 atPos) {
	(void)atPos;
	for (Creature& c : creatures) {
		if (action == "firearm_used") {
			c.knowledge.playerHasFirearms = true;
		} else if (action == "used_tunnel") {
			c.knowledge.playerUsedTunnels = true;
		} else if (action == "ran_away") {
			c.knowledge.playerRanLastEncounter = true;
		} else if (action == "escape_route") {
			c.knowledge.playerHasEscapeRoute = true;
		}

		if (playerId >= 0) {
			c.searchTimer = 0.0f;
		}
	}
}

float CreatureAI::GetNearestCreatureDistance(glm::vec3 fromPos) const {
	float best = std::numeric_limits<float>::max();
	for (const Creature& c : creatures) {
		best = std::min(best, glm::distance(c.position, fromPos));
	}
	if (best == std::numeric_limits<float>::max()) {
		return 1000000.0f;
	}
	return best;
}

Creature* CreatureAI::GetNearestCreature(glm::vec3 fromPos) {
	Creature* best = nullptr;
	float bestDist = std::numeric_limits<float>::max();
	for (Creature& c : creatures) {
		const float dist = glm::distance(c.position, fromPos);
		if (dist < bestDist) {
			bestDist = dist;
			best = &c;
		}
	}
	return best;
}

void CreatureAI::UpdateDormant(Creature& c, float dt) {
	(void)dt;
	c.velocity = glm::vec3(0.0f);
	if (IsNight(dayNight)) {
		TransitionTo(c, CreatureState::PATROL);
	}
}

void CreatureAI::UpdatePatrol(Creature& c, float dt, std::vector<Character*>& players) {
	if (!c.patrolPath.empty()) {
		const glm::vec3 target = c.patrolPath[c.patrolWaypointIndex % c.patrolPath.size()];
		const glm::vec3 toTarget = target - c.position;
		const float dist = glm::length(toTarget);
		if (dist > 0.0001f) {
			const glm::vec3 dir = toTarget / dist;
			c.velocity = dir * 2.5f;
			c.position += c.velocity * dt;
			c.facingAngle = std::atan2(dir.x, dir.z);
		}
		if (dist < 0.5f) {
			c.patrolWaypointIndex = (c.patrolWaypointIndex + 1) % static_cast<int>(c.patrolPath.size());
		}

		// Problem 3: Animation State Switching
		if (c.monsterAnimator && !c.monsterAnimator->IsPlaying(Anim::WALK)) {
			c.monsterAnimator->PlayAnimation(Anim::WALK, true);
		}
	} else {
		if (c.monsterAnimator && !c.monsterAnimator->IsPlaying(Anim::IDLE)) {
			c.monsterAnimator->PlayAnimation(Anim::IDLE, true);
		}
	}

	for (Character* p : players) {
		if (!p || !p->IsAlive()) {
			continue;
		}
		if (CanSeePlayer(c, p) || CanHearPlayer(c, p)) {
			c.lastKnownPlayerPos = p->position;
			TransitionTo(c, CreatureState::HUNT);
			return;
		}
	}

	if (IsNearWindowOrDoorWithoutTalisman(c.position, 3.0f)) {
		TransitionTo(c, CreatureState::WHISPER);
	}
}

void CreatureAI::UpdateSearch(Creature& c, float dt, std::vector<Character*>& players) {
	c.searchTimer += dt;

	RefreshPath(c, c.lastKnownPlayerPos);
	MoveAlongPath(c, 3.5f, dt);

	for (Character* p : players) {
		if (!p || !p->IsAlive()) {
			continue;
		}
		if (CanSeePlayer(c, p) || CanHearPlayer(c, p)) {
			c.lastKnownPlayerPos = p->position;
			TransitionTo(c, CreatureState::HUNT);
			return;
		}
	}

	if (c.searchTimer > 10.0f) {
		TransitionTo(c, CreatureState::PATROL);
	}
}

void CreatureAI::UpdateHunt(Creature& c, float dt, std::vector<Character*>& players) {
	Character* target = FindNearestPlayer(c, players);
	if (!target) {
		TransitionTo(c, CreatureState::SEARCH);
		return;
	}

	c.pathRefreshTimer -= dt;
	if (CanSeePlayer(c, target)) {
		c.lastKnownPlayerPos = target->position;
		c.timeSinceLastSeen = 0.0f;
	} else {
		c.timeSinceLastSeen += dt;
		if (c.timeSinceLastSeen > 8.0f) {
			TransitionTo(c, CreatureState::SEARCH);
			return;
		}
	}

	ApplyKnowledge(c, target);

	glm::vec3 huntGoal = c.lastKnownPlayerPos;
	if (c.knowledge.playerHasFirearms) {
		const glm::vec3 fwd = PlayerForward(target);
		huntGoal = target->position - fwd * 2.0f;
	}
	if (c.knowledge.playerRanLastEncounter) {
		huntGoal = target->position + target->velocity * 1.5f;
	}

	if (c.pathRefreshTimer <= 0.0f) {
		RefreshPath(c, huntGoal);
		c.pathRefreshTimer = 0.5f;
	}

	MoveAlongPath(c, 6.0f, dt);
	HandleTransform(c, dt, true);

	// Problem 3: Animation State Switching (HUNT -> RUN)
	if (c.monsterAnimator && !c.monsterAnimator->IsPlaying(Anim::RUN)) {
		c.monsterAnimator->PlayAnimation(Anim::RUN, true);
	}

	if (glm::distance(c.position, target->position) <= 2.0f) { // Increased range for visual hit
		DealDamageToPlayer(target, 25.0f);
		// Attack animation hook point.
		if (c.monsterAnimator && !c.monsterAnimator->IsPlaying(Anim::ATTACK)) {
			c.monsterAnimator->PlayAnimation(Anim::ATTACK, false);
			c.attackAnimTimer = c.monsterAnimator->GetAnimationDuration(Anim::ATTACK);
		}
	}
}

void CreatureAI::UpdateWhisper(Creature& c, float dt, std::vector<NPC*>& npcs) {
	if (c.whisperTarget == nullptr) {
		float best = std::numeric_limits<float>::max();
		for (NPC* npc : npcs) {
			if (!npc || !IsNpcIndoor(npc)) {
				continue;
			}
			const float d = glm::distance(c.position, glm::vec3(0.0f));
			if (d < 8.0f && d < best) {
				best = d;
				c.whisperTarget = npc;
			}
		}
	}

	c.whisperTimer += dt;
	if (c.whisperTimer >= 3.0f) {
		c.whisperTimer -= 3.0f;
		PlayCreatureWhisperAudio(c.position);
	}

	if (c.whisperTarget != nullptr && NpcOpenedDoorOrWindow(c.whisperTarget)) {
		TransitionTo(c, CreatureState::HUNT);
		return;
	}

	if (SecondsUntilSunrise(dayNight) < 60.0f) {
		TransitionTo(c, CreatureState::RETREAT);
		return;
	}

	if (c.whisperTarget == nullptr) {
		c.searchTimer += dt;
		if (c.searchTimer > 10.0f) {
			TransitionTo(c, CreatureState::PATROL);
		}
	}
}

void CreatureAI::UpdateRetreat(Creature& c, float dt) {
	HandleTransform(c, dt, false);
	c.velocity = glm::vec3(0.0f, 0.0f, -3.5f);
	c.position += c.velocity * dt;

	if (!IsNight(dayNight)) {
		TransitionTo(c, CreatureState::DORMANT);
	}
}

bool CreatureAI::CanSeePlayer(const Creature& c, const Character* player) const {
	if (!player || !player->IsAlive()) {
		return false;
	}

	const glm::vec3 toPlayer = player->position - c.position;
	const float dist = glm::length(toPlayer);
	if (dist > 12.0f) {
		return false;
	}

	const float fovHalf = glm::radians(30.0f);
	const glm::vec3 toPlayerFlat(toPlayer.x, 0.0f, toPlayer.z);
	const float flatDist = glm::length(toPlayerFlat);
	if (flatDist < 0.001f) {
		return true; // Already on top of player
	}
	const glm::vec3 forward(std::sin(c.facingAngle), 0.0f, std::cos(c.facingAngle));
	const float facing = glm::dot(toPlayerFlat / flatDist, forward);
	if (facing < std::cos(fovHalf)) {
		return false;
	}

	return !RayBlockedByWorld(c.position, player->position);
}

bool CreatureAI::CanHearPlayer(const Creature& c, const Character* player) const {
	if (!player || !player->IsAlive()) {
		return false;
	}
	return glm::distance(c.position, player->position) < player->GetNoiseLevelThisFrame();
}

void CreatureAI::RefreshPath(Creature& c, glm::vec3 goal) {
	if (!navMesh) {
		c.currentPath = {goal};
		c.pathIndex = 0;
		return;
	}

	std::vector<glm::vec3> path = navMesh->FindPath(c.position, goal);
	if (path.empty()) {
		c.currentPath = {goal};
	} else {
		c.currentPath = std::move(path);
	}
	c.pathIndex = 0;
}

void CreatureAI::MoveAlongPath(Creature& c, float speed, float dt) {
	if (c.currentPath.empty() || c.pathIndex >= static_cast<int>(c.currentPath.size())) {
		c.velocity = glm::vec3(0.0f);
		return;
	}

	const glm::vec3 target = c.currentPath[c.pathIndex];
	glm::vec3 toTarget = target - c.position;
	const float dist = glm::length(toTarget);

	if (dist < 0.25f) {
		++c.pathIndex;
		if (c.pathIndex >= static_cast<int>(c.currentPath.size())) {
			c.velocity = glm::vec3(0.0f);
			return;
		}
		toTarget = c.currentPath[c.pathIndex] - c.position;
	}

	if (glm::length(toTarget) > 0.0001f) {
		const glm::vec3 dir = glm::normalize(toTarget);
		c.velocity = dir * speed;
		c.position += c.velocity * dt;
		c.facingAngle = std::atan2(dir.x, dir.z);
	} else {
		c.velocity = glm::vec3(0.0f);
	}
}

void CreatureAI::TransitionTo(Creature& c, CreatureState newState) {
	if (c.state == newState) {
		return;
	}

	c.state = newState;
	c.searchTimer = 0.0f;
	c.whisperTimer = 0.0f;
	c.pathRefreshTimer = 0.0f;
	c.pathIndex = 0;
	c.currentPath.clear();

	// Problem 3: Animation Blending on state transition
	if (c.monsterAnimator) {
		const char* animName = Anim::IDLE;
		if (newState == CreatureState::PATROL) animName = Anim::WALK;
		else if (newState == CreatureState::HUNT) animName = Anim::RUN;
		else if (newState == CreatureState::SEARCH) animName = Anim::WALK;
		
		c.monsterAnimator->BlendTo(animName, 0.2f);
	}

	if (newState != CreatureState::WHISPER) {
		c.whisperTarget = nullptr;
	}
}

void CreatureAI::HandleTransform(Creature& c, float dt, bool toMonster) {
	if (toMonster) {
		c.transformProgress += dt / 1.2f;
	} else {
		c.transformProgress -= dt / 0.8f;
	}
	c.transformProgress = std::clamp(c.transformProgress, 0.0f, 1.0f);
	c.isInHumanForm = (c.transformProgress < 0.5f);
	// Renderer should blend human/monster models by transformProgress.
}

void CreatureAI::ApplyKnowledge(Creature& c, Character* target) {
	if (!target) {
		return;
	}

	if (c.knowledge.playerUsedTunnels && c.state == CreatureState::PATROL && navMesh) {
		// Hook: occasionally bias patrol toward known tunnel entrance nodes.
		if (!c.patrolPath.empty()) {
			c.lastKnownPlayerPos = c.patrolPath[c.patrolWaypointIndex % c.patrolPath.size()];
		}
	}

	if (c.knowledge.playerHasEscapeRoute) {
		c.pathRefreshTimer = std::min(c.pathRefreshTimer, 0.25f);
	}
}

