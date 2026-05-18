#include "game/entities/Jade.h"

#include <iostream>
#include "game/quest/Quest.h"

Jade::Jade(glm::vec3 startPos)
    : Character(CharacterType::Jade, "Jade", startPos) {
}

void Jade::LoadDeferredMesh() {
    LoadMesh("assets/models/characters/jade/jade.fbx", "assets/models/characters/jade/jade_Walking.fbx");
}

void Jade::UpdateCharacterState(float dt) {
    // Update vision timer
    if (visionTimer > 0.0f) {
        visionTimer -= dt;
        if (visionTimer <= 0.0f) {
            std::cout << "[Jade] Vision cutscene triggered! Lore fragment received.\n";
            visionTimer = 300.0f;  // Reset to 5 minutes
        }
    }
    
    // Update symbol sight duration
    if (symbolSightActive) {
        symbolSightDuration -= dt;
        if (symbolSightDuration <= 0.0f) {
            symbolSightActive = false;
        }
    }
}

void Jade::OnSwitchedTo() {
    Character::OnSwitchedTo();
    std::cout << "[Jade] Switched to. Glyphs decoded: " << glyphsDecoded << "\n";
}

void Jade::ActivateAbility() {
    if (abilityTimer > 0.0f) return;  // On cooldown
    
    // Toggle Symbol Sight
    symbolSightActive = !symbolSightActive;
    symbolSightDuration = 10.0f;  // 10 seconds active
    abilityTimer = symbolSightCooldown;
    
    if (symbolSightActive && quest) {
        // Each glyph found advances quest
        quest->AdvanceProgress(0.15f);
        glyphsDecoded++;
        std::cout << "[Jade] Glyph #" << glyphsDecoded << " decoded! Symbol Sight ACTIVE\n";
    } else {
        std::cout << "[Jade] Symbol Sight " << (symbolSightActive ? "ACTIVE" : "INACTIVE") << "\n";
    }
}
