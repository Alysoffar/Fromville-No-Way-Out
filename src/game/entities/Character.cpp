#include "game/entities/Character.h"
#include "engine/renderer/Animation.h"
#include "engine/renderer/Animator.h"

#include <iostream>

Character::Character(CharacterType type, std::string name, glm::vec3 startPos)
    : Entity(std::move(name)), type(type), isActive(false), health(100.0f) {
    transform.position = startPos;
}

void Character::Update(float dt) {
    // Base update: apply physics and handle off-screen AI
    ApplyPhysics(dt);
    
    // Update ability cooldown
    if (abilityTimer > 0.0f) {
        abilityTimer -= dt;
    }

    UpdateCharacterState(dt);
}

void Character::UpdateAnimation(float dt) {
    if (!meshLoaded || !animator) return;

    Animation* targetAnim = nullptr;
    if (currentSpeed > 4.0f && runningAnimation && runningAnimation->GetDuration() > 0.0f) {
        targetAnim = runningAnimation.get();
    } else if (currentSpeed > 0.3f && walkingAnimation && walkingAnimation->GetDuration() > 0.0f) {
        targetAnim = walkingAnimation.get();
    } else if (idleAnimation && idleAnimation->GetDuration() > 0.0f) {
        targetAnim = idleAnimation.get();
    }

    if (targetAnim) {
        float animSpeedMultiplier = 1.0f;
        if (targetAnim == runningAnimation.get()) {
            animSpeedMultiplier = currentSpeed / 8.0f;
        } else if (targetAnim == walkingAnimation.get()) {
            animSpeedMultiplier = currentSpeed / 2.0f;
        }
        animSpeedMultiplier = glm::clamp(animSpeedMultiplier, 0.0f, 2.5f);

        animator->SetAnimation(targetAnim);
        animator->UpdateAnimation(dt * animSpeedMultiplier);
    }
}

void Character::OnSwitchedTo() {
    isActive = true;
}

void Character::OnSwitchedFrom() {
    isActive = false;
}

void Character::ActivateAbility() {
    // Override in derived classes
}

void Character::UpdateCharacterState(float dt) {
    (void)dt;
}

glm::vec3 Character::GetDebugColor() const {
    switch (type) {
        case CharacterType::Boyd:    return glm::vec3(1.0f, 0.0f, 0.0f);    // Red
        case CharacterType::Jade:    return glm::vec3(0.0f, 1.0f, 1.0f);    // Cyan
        case CharacterType::Tabitha: return glm::vec3(0.0f, 1.0f, 0.0f);    // Green
        case CharacterType::Victor:  return glm::vec3(1.0f, 1.0f, 0.0f);    // Yellow
        case CharacterType::Sara:    return glm::vec3(1.0f, 0.0f, 1.0f);    // Magenta
    }
    return glm::vec3(0.85f);
}
