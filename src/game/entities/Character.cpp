#include "game/entities/Character.h"

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
