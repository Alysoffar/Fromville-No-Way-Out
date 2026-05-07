#include "game/entities/Victor.h"
#include "game/entities/Victor.h"

#include <iostream>
#include "game/quest/Quest.h"

Victor::Victor(glm::vec3 startPos)
    : Character(CharacterType::Victor, "Victor", startPos) {
}

void Victor::Update(float dt) {
    Character::Update(dt);
    
    // Update memory mode
    if (memoryModeActive) {
        memoryModeDuration -= dt;
        if (memoryModeDuration <= 0.0f) {
            memoryModeActive = false;
        }
    }
}

void Victor::OnSwitchedTo() {
    Character::OnSwitchedTo();
    std::cout << "[Victor] Switched to. Map progress: 80%. Talismans available: " << talismansAvailable << "\n";
}

void Victor::ActivateAbility() {
    if (abilityTimer > 0.0f) return;  // On cooldown
    
    // Enter Memory Mode
    memoryModeActive = true;
    memoryModeDuration = 8.0f;  // 8 seconds of slowed time
    abilityTimer = memoryCooldown;
    
    if (quest) {
        // Each memory recall advances his quest
        quest->AdvanceProgress(0.20f);
        std::cout << "[Victor] Memory unlocked! A fragment of the past becomes clear...\n";
    } else {
        std::cout << "[Victor] Memory Mode activated. Time slows to 20%.\n";
    }
}
