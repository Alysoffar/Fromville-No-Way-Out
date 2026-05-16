#pragma once

#include <nlohmann/json.hpp>

class TruthSystem {
public:
    void AddTruth(float amount);
    void ReduceTruth(float amount);
    float GetTruth() const { return truth; }
    nlohmann::json Serialize() const;
    void Deserialize(const nlohmann::json& j);
private:
    float truth = 0.0f; // 0..100
};
