#include "HallucinationOverlay.h"
#include <iostream>

void HallucinationOverlay::Trigger(float inten) {
    active = true;
    intensity = inten;
}

void HallucinationOverlay::Update(float dt) {
    if (!active) return;
    intensity -= dt * 5.0f;
    if (intensity <= 0.0f) {
        intensity = 0.0f;
        active = false;
    }
}

void HallucinationOverlay::Render() const {
    if (!active) return;
    std::cout << "[HallucinationOverlay] rendering intensity=" << intensity << std::endl;
}

nlohmann::json HallucinationOverlay::Serialize() const {
    return nlohmann::json{{"active", active}, {"intensity", intensity}};
}

void HallucinationOverlay::Deserialize(const nlohmann::json& j) {
    active = j.value("active", false);
    intensity = j.value("intensity", 0.0f);
}
