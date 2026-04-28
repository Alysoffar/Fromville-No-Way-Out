#pragma once

#include "game/entities/Entity.h"

class Player : public Entity {
public:
    Player();

    void Update(float dt) override;

protected:
    float GetMoveSpeed() const override;
    float GetSprintSpeed() const override;
    float GetCrouchSpeed() const override;
    float GetJumpForce() const override;
    float GetGravity() const override;
    float GetFallGravityMultiplier() const override;
    float GetMaxFallSpeed() const override;
    float GetJumpBufferTime() const override;
    float GetCoyoteTime() const override;
    float GetJumpCutMultiplier() const override;
};