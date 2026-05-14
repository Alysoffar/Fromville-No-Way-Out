#pragma once

#include <nlohmann/json.hpp>

class TextRenderer;

/**
 * SanityMeterUI: Visual representation of player sanity/psychological state.
 */
class SanityMeterUI {
public:
    void SetSanity(float sanityLevel);
    void SetTruth(float truthLevel);
    void Update(float dt);
    void Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const;
    nlohmann::json Serialize() const;
    void Deserialize(const nlohmann::json& j);
private:
    float sanity = 100.0f;
    float truth = 0.0f;
};
