#include "game/world/DayNightCycle.h"

#include <cmath>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

DayNightCycle::DayNightCycle(float cycleSpeedSeconds)
    : dayTime(0.25f)
    , cycleSpeed(1.0f / cycleSpeedSeconds)
{
}

void DayNightCycle::update(float deltaTime) {
    dayTime += cycleSpeed * deltaTime;
    dayTime -= std::floor(dayTime);
}

glm::vec3 DayNightCycle::getSunDirection() const {
    float angle = dayTime * 2.0f * glm::pi<float>();
    float y = std::sin(angle);
    float x = std::cos(angle);
    return glm::normalize(glm::vec3(x, y, 0.3f));
}

float DayNightCycle::getDayFactor() const {
    float angle = dayTime * 2.0f * glm::pi<float>();
    float sunY = std::sin(angle);
    return glm::clamp(sunY * 2.0f + 0.5f, 0.0f, 1.0f);
}

glm::vec3 DayNightCycle::getLightColor() const {
    float factor = getDayFactor();
    glm::vec3 night(0.3f, 0.35f, 0.5f);
    glm::vec3 day(1.0f, 0.95f, 0.80f);
    return glm::mix(night, day, factor);
}

glm::vec3 DayNightCycle::getAmbientColor() const {
    float factor = getDayFactor();
    glm::vec3 night(0.04f, 0.04f, 0.07f);
    glm::vec3 day(0.35f, 0.32f, 0.28f);
    return glm::mix(night, day, factor);
}

float DayNightCycle::getDiffuseStrength() const {
    float factor = getDayFactor();
    return glm::mix(0.3f, 0.85f, factor);
}

glm::vec3 DayNightCycle::getFogColor() const {
    float factor = getDayFactor();
    glm::vec3 night(0.02f, 0.02f, 0.04f);
    glm::vec3 day(0.55f, 0.62f, 0.72f);
    return glm::mix(night, day, factor);
}

glm::vec3 DayNightCycle::getActiveLightDir() const {
    glm::vec3 sunDir = getSunDirection();
    return (getDayFactor() > 0.1f) ? sunDir : -sunDir;
}

float DayNightCycle::getDayTime() const {
    return dayTime;
}
