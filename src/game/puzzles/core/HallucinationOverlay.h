#pragma once

#include <nlohmann/json.hpp>

class HallucinationOverlay {
public:
    void Trigger(float intensity);
    void Update(float dt);
    void Render() const;
    bool IsActive() const { return active; }
    nlohmann::json Serialize() const;
    void Deserialize(const nlohmann::json& j);
private:
    bool active = false;
    float intensity = 0.0f;
};
