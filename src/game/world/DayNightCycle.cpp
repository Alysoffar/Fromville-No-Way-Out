#include "game/world/DayNightCycle.h"

#include <cmath>

#include <glm/gtc/constants.hpp>

DayNightCycle::DayNightCycle(float cycleSpeedSeconds)
    : dayTime(0.375f)  // start at bright morning
    , cycleSpeed(1.0f / cycleSpeedSeconds)
    , cycleSeconds(cycleSpeedSeconds)
{
}

void DayNightCycle::update(float deltaTime) {
    dayTime += cycleSpeed * deltaTime;
    // Wrap at 1.0
    dayTime -= std::floor(dayTime);
}

void DayNightCycle::syncToWorldClock(float worldClockSeconds) {
    float cyclePosition = std::fmod(worldClockSeconds, cycleSeconds);
    if (cyclePosition < 0.0f) {
        cyclePosition += cycleSeconds;
    }

    dayTime = 0.25f + (cyclePosition / cycleSeconds);
    dayTime -= std::floor(dayTime);
}

glm::vec3 DayNightCycle::getSunDirection() const {
    // Map dayTime so 0.25 = sunrise, 0.5 = noon, 0.75 = sunset, 0.0 = midnight.
    float angle = (dayTime - 0.25f) * 2.0f * glm::pi<float>();
    float y = std::sin(angle);
    float x = std::cos(angle);
    return glm::normalize(glm::vec3(x, y, 0.3f));
}

float DayNightCycle::getDayFactor() const {
    // Daylight intensity is based on the sun being above the horizon.
    // 0.25=sunrise, 0.5=noon, 0.75=sunset.
    float angle = (dayTime - 0.25f) * 2.0f * glm::pi<float>();
    float sunY = std::sin(angle);
    return glm::smoothstep(0.0f, 1.0f, glm::clamp(sunY, 0.0f, 1.0f));
}

glm::vec3 DayNightCycle::getLightColor() const {
    float factor = getDayFactor();
    glm::vec3 nightColor(0.6f, 0.65f, 0.8f);
    glm::vec3 dayColor(1.0f, 0.95f, 0.80f);
    return glm::mix(nightColor, dayColor, factor);
}

glm::vec3 DayNightCycle::getAmbientColor() const {
    float factor = getDayFactor();
    glm::vec3 nightAmbient(0.15f, 0.15f, 0.2f);
    glm::vec3 dayAmbient(0.35f, 0.32f, 0.28f);
    return glm::mix(nightAmbient, dayAmbient, factor);
}

float DayNightCycle::getDiffuseStrength() const {
    float factor = getDayFactor();
    return glm::mix(0.5f, 0.85f, factor);
}

glm::vec3 DayNightCycle::getFogColor() const {
    float factor = getDayFactor();
    glm::vec3 nightFog(0.1f, 0.1f, 0.15f);
    glm::vec3 dayFog(0.55f, 0.62f, 0.72f);
    return glm::mix(nightFog, dayFog, factor);
}

glm::vec3 DayNightCycle::getActiveLightDir() const {
    glm::vec3 sunDir = getSunDirection();
    float angle = (dayTime - 0.25f) * 2.0f * glm::pi<float>();
    float sunY = std::sin(angle);
    // During day, use sun direction; during night, flip to moonlight.
    if (sunY > 0.0f) {
        return sunDir;
    } else {
        return -sunDir;
    }
}

float DayNightCycle::getDayTime() const {
    return dayTime;
}
