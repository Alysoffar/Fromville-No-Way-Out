#include "game/entities/Boyd.h"
#include "game/entities/Boyd.h"

#include <iostream>
#include "game/quest/Quest.h"
#include <glm/gtc/type_ptr.hpp>

Boyd::Boyd(glm::vec3 startPos)
    : Character(CharacterType::Boyd, "Boyd", startPos) {
}

void Boyd::Update(float dt) {
    Character::Update(dt);
    
    // Update curse level decay
    if (curseLevel > 0.0f && !isRaging) {
        curseLevel -= dt * 5.0f;  // Decay over time
    }
    
    // Update rage timer
    if (isRaging) {
        rageDuration -= dt;
        if (rageDuration <= 0.0f) {
            isRaging = false;
        }
    }
    
    // Check if curse meter is full, trigger rage
    if (curseLevel >= 100.0f && !isRaging) {
        isRaging = true;
        rageDuration = 3.0f;  // 3 seconds of uncontrol
    }
}

void Boyd::OnSwitchedTo() {
    Character::OnSwitchedTo();
    std::cout << "[Boyd] Switched to. Curse level: " << curseLevel << "\n";
}

void Boyd::ActivateAbility() {
    if (abilityTimer > 0.0f) return;  // On cooldown
    
    // Interrogate NPCs within 3m
    abilityTimer = 8.0f;  // 8 second cooldown
    
    // Advance quest objective (interrogate suspects)
    if (quest) {
        quest->AdvanceProgress(0.10f);  // Each interrogation is 10% progress
        std::cout << "[Boyd] Interrogating suspects... They reveal secrets.\n";
    } else {
        std::cout << "[Boyd] Interrogating nearby NPCs...\n";
    }
}
