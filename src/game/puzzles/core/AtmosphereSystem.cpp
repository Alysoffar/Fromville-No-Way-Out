#include "AtmosphereSystem.h"

void AtmosphereSystem::Pulse(float amount) {
    level += amount;
    if (level > 100.0f) level = 100.0f;
}

void AtmosphereSystem::Escalate(float /*dt*/) {
    // Passive decay/settle could be implemented
}

nlohmann::json AtmosphereSystem::Serialize() const {
    return nlohmann::json{{"level", level}};
}

void AtmosphereSystem::Deserialize(const nlohmann::json& j) {
    level = j.value("level", 0.0f);
}
