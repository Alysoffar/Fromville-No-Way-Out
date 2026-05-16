#pragma once

#include <glm/glm.hpp>

class DayNightCycle {
public:
    explicit DayNightCycle(float cycleSpeedSeconds = 120.0f);

    void update(float deltaTime);

    glm::vec3 getSunDirection() const;
    glm::vec3 getLightColor() const;
    glm::vec3 getAmbientColor() const;
    float getDiffuseStrength() const;
    glm::vec3 getFogColor() const;
    glm::vec3 getActiveLightDir() const;
    float getDayFactor() const;
    float getDayTime() const;

private:
    float dayTime;     // 0.0 to 1.0
    float cycleSpeed;  // fraction per second = 1.0 / cycleSpeedSeconds
};
