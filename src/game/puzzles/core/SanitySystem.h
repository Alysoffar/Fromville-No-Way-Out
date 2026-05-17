#pragma once

#include <nlohmann/json.hpp>

class SanitySystem {
public:
    void AddPressure(float amount);
    void Reduce(float amount);
    void Update(float dt);
    float GetSanity() const { return sanity; }
    bool IsBroken() const { return sanity <= 0.0f; }
    nlohmann::json Serialize() const;
    void Deserialize(const nlohmann::json& j);
private:
    float sanity = 100.0f; // 0..100
};
