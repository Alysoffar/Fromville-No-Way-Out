#pragma once

#include "game/entities/Character.h"

class Victor : public Character {
public:
    explicit Victor(glm::vec3 startPos = glm::vec3(0.0f));

    void OnSwitchedTo() override;
    void ActivateAbility() override;  // Memory Mode
    void LoadDeferredMesh() override;

    bool IsInMemoryMode() const { return memoryModeActive; }
    float GetDetectionRadius() const { return 8.0f; }  // Smallest of all
    int GetMapProgress() const { return 80; }  // Starts with 80%

protected:
    void UpdateCharacterState(float dt) override;
    float GetMoveSpeed() const override { return 7.0f; }

private:
    bool memoryModeActive = false;
    float memoryModeDuration = 0.0f;
    float timeSlowFactor = 0.2f;  // Slows to 20%
    float memoryCooldown = 30.0f;
    int talismansAvailable = 3;
};
