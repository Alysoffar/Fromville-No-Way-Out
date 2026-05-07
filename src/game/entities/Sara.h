#pragma once

#include "game/entities/Character.h"

class Sara : public Character {
public:
    explicit Sara(glm::vec3 startPos = glm::vec3(0.0f));

    void Update(float dt) override;
    void OnSwitchedTo() override;
    void ActivateAbility() override;  // Ghost Step

    bool IsGhostStepping() const { return ghostStepActive; }
    float GetRedemptionScore() const { return redemptionScore; }
    void AddRedemption(float amount) { redemptionScore = glm::clamp(redemptionScore + amount, 0.0f, 100.0f); }

protected:
    float GetMoveSpeed() const override { return 8.5f; }

private:
    bool ghostStepActive = false;
    float ghostStepDuration = 0.0f;
    float ghostStepCooldown = 30.0f;
    float redemptionScore = 0.0f;  // Affects false positive rate
    float warningRange = 25.0f;
    float warningLeadTime = 3.5f;  // 3-5 seconds advance warning
};
