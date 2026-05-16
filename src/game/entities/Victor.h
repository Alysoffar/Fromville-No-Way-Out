#pragma once

#include "game/entities/Character.h"

class Victor : public Character {
public:
    explicit Victor(glm::vec3 startPos = glm::vec3(0.0f));

    void OnSwitchedTo() override;
    void ActivateAbility() override;  // Memory Mode

protected:
    void UpdateCharacterState(float dt) override;
    float GetMoveSpeed() const override { return 7.0f; }

private:
    float memoryModeDuration = 5.0f;
    float memoryRechargeTimer = 0.0f;
};
