#include "game/puzzles/ui/SanityMeterUI.h"

void SanityMeterUI::SetSanity(float sanityLevel) {
    sanity = sanityLevel;
    if (sanity < 0.0f) sanity = 0.0f;
    if (sanity > 100.0f) sanity = 100.0f;
}

void SanityMeterUI::SetTruth(float truthLevel) {
    truth = truthLevel;
    if (truth < 0.0f) truth = 0.0f;
    if (truth > 100.0f) truth = 100.0f;
}

void SanityMeterUI::Update(float /*dt*/) {
    // Animation logic here (e.g., pulse effects, color transitions)
}

void SanityMeterUI::Render(TextRenderer& /*textRenderer*/, int /*screenWidth*/, int /*screenHeight*/, float /*alpha*/) const {
    // Placeholder: Real rendering would display bars or visual indicators
}

nlohmann::json SanityMeterUI::Serialize() const {
    return nlohmann::json{{"sanity", sanity}, {"truth", truth}};
}

void SanityMeterUI::Deserialize(const nlohmann::json& j) {
    sanity = j.value("sanity", 100.0f);
    truth = j.value("truth", 0.0f);
}
