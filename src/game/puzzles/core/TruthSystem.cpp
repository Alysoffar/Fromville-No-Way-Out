#include "TruthSystem.h"

void TruthSystem::AddTruth(float amount) {
    truth += amount;
    if (truth > 100.0f) truth = 100.0f;
}

void TruthSystem::ReduceTruth(float amount) {
    truth -= amount;
    if (truth < 0.0f) truth = 0.0f;
}

nlohmann::json TruthSystem::Serialize() const {
    return nlohmann::json{{"truth", truth}};
}

void TruthSystem::Deserialize(const nlohmann::json& j) {
    if (j.contains("truth")) truth = j.at("truth").get<float>();
}
