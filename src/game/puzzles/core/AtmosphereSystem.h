#pragma once

#include <nlohmann/json.hpp>

class AtmosphereSystem {
public:
    void Pulse(float amount);
    void Escalate(float dt);
    float GetLevel() const { return level; }
    nlohmann::json Serialize() const;
    void Deserialize(const nlohmann::json& j);
private:
    float level = 0.0f; // 0..100
};
