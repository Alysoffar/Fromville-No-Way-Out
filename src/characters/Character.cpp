#include "Character.h"

#include <algorithm>

#include "renderer/ProceduralHumanoid.h"
#include "renderer/ProceduralHair.h"
#include "renderer/ProceduralClothing.h"
#include "renderer/FaceDetailGenerator.h"
#include "renderer/AnimationNames.h"

Character::~Character() {
	delete proceduralHumanoid;
	proceduralHumanoid = nullptr;
	delete proceduralHair;
	proceduralHair = nullptr;
	delete proceduralClothing;
	proceduralClothing = nullptr;
	delete faceDetails;
	faceDetails = nullptr;
}

PostFXState Character::GetPostFXState() const {
	PostFXState fx;
	fx.isUnderground = false;
	return fx;
}

void Character::OnBecomeActive() {
	isActivePlayer = true;
}

void Character::OnBecomeInactive() {
	isActivePlayer = false;
	state = CharacterState::IDLE_OFFSCREEN;
}

void Character::Move(glm::vec3 dir, float speed, float dt) {
	if (!IsAlive()) {
		return;
	}

	glm::vec3 flatDir(dir.x, 0.0f, dir.z);
	if (glm::length(flatDir) > 0.0001f) {
		flatDir = glm::normalize(flatDir);
	}

	position += flatDir * speed * dt;
	position.y += velocity.y * dt;

	if (position.y < 0.0f) {
		position.y = 0.0f;
		velocity.y = 0.0f;
	}
}

void Character::TakeDamage(float amount) {
	if (amount <= 0.0f || IsDead()) {
		return;
	}

	stats.health = std::max(0.0f, stats.health - amount);
	state = (stats.health <= 0.0f) ? CharacterState::DEAD : CharacterState::HURT;
	if (state == CharacterState::DEAD) {
		currentNoiseRadius = 0.0f;
	}
}

void Character::Heal(float amount) {
	if (amount <= 0.0f || IsDead()) {
		return;
	}
	stats.health = std::min(stats.maxHealth, stats.health + amount);
}

void Character::UseStamina(float amount) {
	if (amount <= 0.0f) {
		return;
	}
	stats.stamina = std::max(0.0f, stats.stamina - amount);
}

void Character::UpdateAnimationState() {
	switch (state) {
	case CharacterState::RUNNING:
		currentNoiseRadius = 8.0f;
		if (animator && !animator->IsPlaying(Anim::RUN)) {
			animator->PlayAnimation(Anim::RUN, true);
		}
		break;
	case CharacterState::CROUCHING:
		currentNoiseRadius = 1.0f;
		if (animator && !animator->IsPlaying(Anim::CROUCH)) {
			animator->PlayAnimation(Anim::CROUCH, true);
		}
		break;
	case CharacterState::WALKING:
		currentNoiseRadius = 3.0f;
		if (animator && !animator->IsPlaying(Anim::WALK)) {
			animator->PlayAnimation(Anim::WALK, true);
		}
		break;
	case CharacterState::DEAD:
		currentNoiseRadius = 0.0f;
		if (animator && !animator->IsPlaying(Anim::DEATH)) {
			animator->PlayAnimation(Anim::DEATH, false);
		}
		break;
	case CharacterState::HURT:
		currentNoiseRadius = 1.0f;
		if (animator && !animator->IsPlaying(Anim::HURT)) {
			animator->PlayAnimation(Anim::HURT, false);
		}
		break;
	case CharacterState::IDLE:
	case CharacterState::IDLE_OFFSCREEN:
	default:
		currentNoiseRadius = 0.0f;
		if (animator && !animator->IsPlaying(Anim::IDLE)) {
			animator->PlayAnimation(Anim::IDLE, true);
		}
		break;
	}
}

void Character::SimulateOffscreen(float dt) {
	if (state != CharacterState::IDLE_OFFSCREEN || IsDead()) {
		return;
	}

	stats.stamina = std::min(stats.maxStamina, stats.stamina + stats.staminaRegenRate * dt * 0.5f);
	velocity = glm::vec3(0.0f);
	currentNoiseRadius = 0.0f;
}

