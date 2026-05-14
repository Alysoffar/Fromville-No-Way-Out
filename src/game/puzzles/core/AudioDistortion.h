#pragma once

#include <nlohmann/json.hpp>

class AudioDistortion {
public:
    void SetAmount(float a) { amount = a; }
    float GetAmount() const { return amount; }
    nlohmann::json Serialize() const { return nlohmann::json{{"amount", amount}}; }
    void Deserialize(const nlohmann::json& j) { amount = j.value("amount", 0.0f); }
private:
    float amount = 0.0f;
};
