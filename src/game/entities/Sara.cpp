#include "game/entities/Sara.h"

#include <iostream>
#include "game/quest/Quest.h"

#include <filesystem>

Sara::Sara(glm::vec3 startPos)
    : Character(CharacterType::Sara, "Sara", startPos) {
    if (std::filesystem::exists("assets/models/characters/sara/sara.fbx")) {
        LoadMesh("assets/models/characters/sara/sara.fbx", "assets/models/characters/sara/sara_Walking.fbx");
    } else {
        LoadMesh("assets/models/characters/jade/jade.fbx", "assets/models/characters/jade/jade_Walking.fbx");
    }
}

void Sara::UpdateCharacterState(float dt) {
    // Update ghost step
    if (ghostStepActive) {
        ghostStepDuration -= dt;
        if (ghostStepDuration <= 0.0f) {
            ghostStepActive = false;
        }
    }
}

void Sara::OnSwitchedTo() {
    Character::OnSwitchedTo();
    std::cout << "[Sara] Switched to. Redemption Score: " << redemptionScore << "\n";
}

void Sara::ActivateAbility() {
    if (abilityTimer > 0.0f) return;  // On cooldown
    
    // Activate Ghost Step
    ghostStepActive = true;
    ghostStepDuration = 4.0f;  // 4 seconds undetectable
    abilityTimer = ghostStepCooldown;
    
    if (quest) {
        // Each Ghost Step is a step toward truth
        quest->AdvanceProgress(0.12f);
        AddRedemption(5.0f);
        std::cout << "[Sara] Ghost Step activated! Moving unseen toward the truth...\n";
    } else {
        std::cout << "[Sara] Ghost Step activated! Fully undetectable for 4 seconds.\n";
    }
}
