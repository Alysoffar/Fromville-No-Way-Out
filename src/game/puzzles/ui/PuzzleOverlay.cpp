#include "game/puzzles/ui/PuzzleOverlay.h"

void PuzzleOverlay::Activate() {
    active = true;
    intensity = 1.0f;
}

void PuzzleOverlay::Update(float dt) {
    if (!active) return;
    intensity -= dt * 2.0f;
    if (intensity <= 0.0f) {
        intensity = 0.0f;
        active = false;
    }
}

void PuzzleOverlay::Render(TextRenderer& /*textRenderer*/, int /*screenWidth*/, int /*screenHeight*/, float /*alpha*/) const {
    if (!active) return;
    // Placeholder: Real rendering would use fullscreen quads or shader effects
}

nlohmann::json PuzzleOverlay::Serialize() const {
    return nlohmann::json{{"active", active}, {"intensity", intensity}};
}

void PuzzleOverlay::Deserialize(const nlohmann::json& j) {
    active = j.value("active", false);
    intensity = j.value("intensity", 0.0f);
}
