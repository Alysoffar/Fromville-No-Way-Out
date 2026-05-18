#pragma once

#include "game/entities/Character.h"

class Boyd : public Character {
public:
    explicit Boyd(glm::vec3 startPos = glm::vec3(0.0f));

    void OnSwitchedTo() override;
    void ActivateAbility() override;  // Interrogate NPCs
    void LoadDeferredMesh() override;

    float GetCurseLevel() const { return curseLevel; }
    void AddCurseCharge(float amount) { curseLevel = glm::clamp(curseLevel + amount, 0.0f, 100.0f); }
    bool IsRaging() const { return isRaging; }

protected:
    void UpdateCharacterState(float dt) override;
    float GetMoveSpeed() const override { return 10.0f; }

private:
    float curseLevel = 0.0f;      // 0-100, rage boost at 100
    bool isRaging = false;        // Temporary uncontrollable state
    float rageDuration = 0.0f;    // Time left in rage
    float interrogationRange = 3.0f;
};
