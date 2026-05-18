#pragma once

#include "game/entities/Character.h"

class Tabitha : public Character {
public:
    explicit Tabitha(glm::vec3 startPos = glm::vec3(0.0f));

    void OnSwitchedTo() override;
    void ActivateAbility() override;  // Listen at walls
    void LoadDeferredMesh() override;

    int GetTunnelsExplored() const { return tunnelsExplored; }
    bool HasListeningAbility() const { return true; }

protected:
    float GetMoveSpeed() const override { return 9.0f; }

private:
    int tunnelsExplored = 0;
    float listeningRange = 5.0f;
    float listeningCooldown = 8.0f;
    float lootMultiplier = 1.5f;  // 50% more items found
};
