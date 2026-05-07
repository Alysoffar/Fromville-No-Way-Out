#pragma once

#include "game/entities/Character.h"

class Jade : public Character {
public:
    explicit Jade(glm::vec3 startPos = glm::vec3(0.0f));

    void Update(float dt) override;
    void OnSwitchedTo() override;
    void ActivateAbility() override;  // Symbol Sight

    bool IsSymbolSightActive() const { return symbolSightActive; }
    float GetVisionTimer() const { return visionTimer; }
    int GetGlyphsDecoded() const { return glyphsDecoded; }

protected:
    float GetMoveSpeed() const override { return 8.0f; }

private:
    bool symbolSightActive = false;
    float symbolSightDuration = 0.0f;
    float visionTimer = 300.0f;  // 5 minutes between visions
    int glyphsDecoded = 0;       // Persistent glyph progress
    float symbolSightCooldown = 15.0f;
};
