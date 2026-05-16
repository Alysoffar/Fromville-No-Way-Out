#pragma once

#include <nlohmann/json.hpp>

class TextRenderer;

/**
 * PuzzleOverlay: Visual overlay for cinematic puzzle transitions and effects.
 */
class PuzzleOverlay {
public:
    void Activate();
    void Update(float dt);
    void Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const;
    void SetIntensity(float intensity) { this->intensity = intensity; }
    nlohmann::json Serialize() const;
    void Deserialize(const nlohmann::json& j);
private:
    bool active = false;
    float intensity = 0.0f;
};
