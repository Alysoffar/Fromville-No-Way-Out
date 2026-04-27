#include "game/entities/NPC.h"

#include <utility>

NPC::NPC(std::string displayName)
    : Entity(std::move(displayName)) {
}

void NPC::Update(float dt) {
    (void)dt;
}