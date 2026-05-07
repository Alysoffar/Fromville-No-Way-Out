#include "game/entities/Tabitha.h"
#include "game/entities/Tabitha.h"

#include <iostream>
#include "game/quest/Quest.h"

Tabitha::Tabitha(glm::vec3 startPos)
    : Character(CharacterType::Tabitha, "Tabitha", startPos) {
}

void Tabitha::Update(float dt) {
    Character::Update(dt);
}

void Tabitha::OnSwitchedTo() {
    Character::OnSwitchedTo();
    std::cout << "[Tabitha] Switched to. Tunnels explored: " << tunnelsExplored << "\n";
}

void Tabitha::ActivateAbility() {
    if (abilityTimer > 0.0f) return;  // On cooldown
    
    // Listen at walls for creatures
    abilityTimer = listeningCooldown;
    
    if (quest) {
        // Listening reveals a new tunnel section
        tunnelsExplored++;
        quest->AdvanceProgress(0.15f);
        std::cout << "[Tabitha] Tunnel section #" << tunnelsExplored << " mapped! Creatures detected.\n";
    } else {
        std::cout << "[Tabitha] Listening for creatures nearby...\n";
    }
}
