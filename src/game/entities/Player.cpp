#include "game/entities/Player.h"

Player::Player()
    : Character(CharacterType::Boyd, "Player") {
}

void Player::Update(float dt) {
    Character::Update(dt);
}