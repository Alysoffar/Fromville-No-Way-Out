#include "SanitySystem.h"

void SanitySystem::AddPressure(float amount) {
    sanity -= amount;
    if (sanity < 0.0f) sanity = 0.0f;
}

void SanitySystem::Reduce(float amount) {
    sanity += amount;
    if (sanity > 100.0f) sanity = 100.0f;
}

void SanitySystem::Update(float /*dt*/) {
    // Passive recovery could be added here
}

nlohmann::json SanitySystem::Serialize() const {
    return nlohmann::json{{"sanity", sanity}};
}

void SanitySystem::Deserialize(const nlohmann::json& j) {
    if (j.contains("sanity")) sanity = j.at("sanity").get<float>();
}
