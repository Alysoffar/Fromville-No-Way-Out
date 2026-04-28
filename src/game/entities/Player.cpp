#include "game/entities/Player.h"

Player::Player()
    : Entity("Player") {
}

float Player::GetMoveSpeed() const {
    return 12.0f;
}

float Player::GetSprintSpeed() const {
    return 20.0f;
}

float Player::GetCrouchSpeed() const {
    return 4.5f;
}

float Player::GetJumpForce() const {
    return 12.5f;
}

float Player::GetGravity() const {
    return -32.0f;
}

float Player::GetFallGravityMultiplier() const {
    return 2.35f;
}

float Player::GetMaxFallSpeed() const {
    return -48.0f;
}

float Player::GetJumpBufferTime() const {
    return 0.14f;
}

float Player::GetCoyoteTime() const {
    return 0.12f;
}

float Player::GetJumpCutMultiplier() const {
    return 0.50f;
}

void Player::Update(float dt) {
    ApplyPhysics(dt);
}