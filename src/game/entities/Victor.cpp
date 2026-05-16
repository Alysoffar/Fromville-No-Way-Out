#include "game/entities/Victor.h"

#include <iostream>
#include "game/memory/MemoryReplaySystem.h"

Victor::Victor(glm::vec3 startPos)
    : Character(CharacterType::Victor, "Victor", startPos) {
}

void Victor::UpdateCharacterState(float dt) {
    if (memoryRechargeTimer > 0.0f) {
        memoryRechargeTimer -= dt;
    }
}

void Victor::OnSwitchedTo() {
    Character::OnSwitchedTo();
    std::cout << "[Victor] Switched to. Memories are waiting.\n";
}

void Victor::ActivateAbility() {
    if (memoryRechargeTimer > 0.0f) return;
    
    std::cout << "[Victor] Activating Memory Mode... The past surfaces.\n";
    MemoryReplaySystem::Instance().ActivateMemoryMode(memoryModeDuration);
    memoryRechargeTimer = 15.0f;  // 15 second recharge
}
