#include "DayNightCycle.h"

#include <algorithm>
#include <array>
#include <cmath>

#include <glm/gtc/constants.hpp>

namespace {

constexpr std::array<float, 4> kLoopPhaseWeights = {{0.50f, 0.16f, 0.24f, 0.10f}};

float NormalizeTime(float t) {
    if (t > 1.0001f) {
        t /= 24.0f;
    }

    t = std::fmod(t, 1.0f);
    if (t < 0.0f) {
        t += 1.0f;
    }
    return t;
}

float InvLerp(float a, float b, float v) {
    if (std::abs(b - a) < 1e-6f) {
        return 0.0f;
    }
    return glm::clamp((v - a) / (b - a), 0.0f, 1.0f);
}

} // namespace

namespace {

void ApplyLoopPhasePreset(GameLoopPhase phase, float& timeOfDay) {
    switch (phase) {
    case GameLoopPhase::Explore:
        timeOfDay = 0.50f;
        break;
    case GameLoopPhase::Prepare:
        timeOfDay = 0.74f;
        break;
    case GameLoopPhase::Survive:
        timeOfDay = 0.92f;
        break;
    case GameLoopPhase::SwitchCharacter:
    default:
        timeOfDay = 0.28f;
        break;
    }
}

float GetLoopPhaseWeight(GameLoopPhase phase) {
    switch (phase) {
    case GameLoopPhase::Explore: return kLoopPhaseWeights[0];
    case GameLoopPhase::Prepare: return kLoopPhaseWeights[1];
    case GameLoopPhase::Survive: return kLoopPhaseWeights[2];
    case GameLoopPhase::SwitchCharacter:
    default: return kLoopPhaseWeights[3];
    }
}

GameLoopPhase AdvanceLoopPhase(GameLoopPhase phase) {
    switch (phase) {
    case GameLoopPhase::Explore: return GameLoopPhase::Prepare;
    case GameLoopPhase::Prepare: return GameLoopPhase::Survive;
    case GameLoopPhase::Survive: return GameLoopPhase::SwitchCharacter;
    case GameLoopPhase::SwitchCharacter:
    default: return GameLoopPhase::Explore;
    }
}

const char* LoopPhaseName(GameLoopPhase phase) {
    switch (phase) {
    case GameLoopPhase::Explore: return "Explore";
    case GameLoopPhase::Prepare: return "Prepare";
    case GameLoopPhase::Survive: return "Survive";
    case GameLoopPhase::SwitchCharacter:
    default: return "Switch Character";
    }
}

} // namespace

DayNightCycle& DayNightCycle::Get() {
    static DayNightCycle instance;
    return instance;
}

bool DayNightCycle::IsDaytime() const {
    return loopPhase == GameLoopPhase::Explore || loopPhase == GameLoopPhase::SwitchCharacter;
}

void DayNightCycle::SetTime(float t) {
    timeOfDay = NormalizeTime(t);
}

float DayNightCycle::GetTimeOfDay() const {
    return timeOfDay;
}

void DayNightCycle::SetPhase(DayPhase phase) {
    switch (phase) {
    case DayPhase::Dawn:
        SetLoopPhase(GameLoopPhase::SwitchCharacter);
        break;
    case DayPhase::Day:
        SetLoopPhase(GameLoopPhase::Explore);
        break;
    case DayPhase::Dusk:
        SetLoopPhase(GameLoopPhase::Prepare);
        break;
    case DayPhase::Night:
    default:
        SetLoopPhase(GameLoopPhase::Survive);
        break;
    }
}

DayPhase DayNightCycle::GetPhase() const {
    if (timeOfDay >= 0.2f && timeOfDay < 0.3f) {
        return DayPhase::Dawn;
    }
    if (timeOfDay >= 0.3f && timeOfDay < 0.7f) {
        return DayPhase::Day;
    }
    if (timeOfDay >= 0.7f && timeOfDay < 0.8f) {
        return DayPhase::Dusk;
    }
    return DayPhase::Night;
}

const char* DayNightCycle::GetPhaseName() const {
    return LoopPhaseName(loopPhase);
}

void DayNightCycle::SetLoopPhase(GameLoopPhase phase) {
    loopPhase = phase;
    loopPhaseElapsed = 0.0f;
    ApplyLoopPhasePreset(loopPhase, timeOfDay);
}

GameLoopPhase DayNightCycle::GetLoopPhase() const {
    return loopPhase;
}

void DayNightCycle::SetAutoAdvance(bool enabled) {
    autoAdvance = enabled;
}

void DayNightCycle::ToggleAutoAdvance() {
    autoAdvance = !autoAdvance;
}

bool DayNightCycle::IsAutoAdvanceEnabled() const {
    return autoAdvance;
}

void DayNightCycle::SetCycleDurationSeconds(float seconds) {
    cycleDurationSeconds = std::max(30.0f, seconds);
}

float DayNightCycle::GetCycleDurationSeconds() const {
    return cycleDurationSeconds;
}

float DayNightCycle::GetDaylightFactor() const {
    const float rise = glm::smoothstep(0.14f, 0.28f, timeOfDay);
    const float fall = 1.0f - glm::smoothstep(0.72f, 0.88f, timeOfDay);
    return glm::clamp(rise * fall, 0.0f, 1.0f);
}

glm::vec3 DayNightCycle::GetSunDirection() const {
    const float orbit = (timeOfDay - 0.25f) * glm::two_pi<float>();
    const float height = std::sin(orbit);
    const float horizontalX = std::cos(orbit) * 0.42f;
    const float horizontalZ = std::sin(orbit * 0.6f + 0.7f) * 0.28f;

    if (height >= 0.0f) {
        return glm::normalize(glm::vec3(horizontalX, -std::max(height, 0.22f), horizontalZ));
    }

    return glm::normalize(glm::vec3(-horizontalX * 0.65f, -std::max(-height, 0.35f), -horizontalZ * 0.75f));
}

glm::vec3 DayNightCycle::GetAmbientColor() const {
    const float daylight = GetDaylightFactor();

    if (timeOfDay >= 0.14f && timeOfDay < 0.28f) {
        const float t = InvLerp(0.14f, 0.28f, timeOfDay);
        return glm::mix(glm::vec3(0.14f, 0.13f, 0.16f), glm::vec3(0.62f, 0.66f, 0.70f), t);
    }
    if (timeOfDay >= 0.72f && timeOfDay < 0.88f) {
        const float t = InvLerp(0.72f, 0.88f, timeOfDay);
        return glm::mix(glm::vec3(0.40f, 0.40f, 0.44f), glm::vec3(0.12f, 0.10f, 0.12f), t);
    }

    return glm::mix(glm::vec3(0.08f, 0.07f, 0.10f), glm::vec3(0.74f, 0.77f, 0.80f), daylight);
}

glm::vec3 DayNightCycle::GetDirectionalLightColor() const {
    if (timeOfDay >= 0.14f && timeOfDay < 0.28f) {
        const float t = InvLerp(0.14f, 0.28f, timeOfDay);
        return glm::mix(glm::vec3(1.05f, 0.70f, 0.32f), glm::vec3(1.08f, 1.08f, 1.04f), t);
    }
    if (timeOfDay >= 0.28f && timeOfDay < 0.72f) {
        const float midday = glm::smoothstep(0.28f, 0.45f, timeOfDay) * (1.0f - glm::smoothstep(0.56f, 0.72f, timeOfDay));
        return glm::mix(glm::vec3(1.10f, 1.08f, 1.03f), glm::vec3(1.02f, 1.00f, 0.96f), midday * 0.12f);
    }
    if (timeOfDay >= 0.72f && timeOfDay < 0.88f) {
        const float t = InvLerp(0.72f, 0.88f, timeOfDay);
        return glm::mix(glm::vec3(1.02f, 0.66f, 0.24f), glm::vec3(0.30f, 0.34f, 0.48f), t);
    }
    return glm::vec3(0.16f, 0.20f, 0.32f);
}

glm::vec3 DayNightCycle::GetFogColor() const {
    if (timeOfDay >= 0.14f && timeOfDay < 0.28f) {
        const float t = InvLerp(0.14f, 0.28f, timeOfDay);
        return glm::mix(glm::vec3(0.56f, 0.50f, 0.46f), glm::vec3(0.90f, 0.93f, 0.96f), t);
    }
    if (timeOfDay >= 0.72f && timeOfDay < 0.88f) {
        const float t = InvLerp(0.72f, 0.88f, timeOfDay);
        return glm::mix(glm::vec3(0.84f, 0.54f, 0.34f), glm::vec3(0.20f, 0.24f, 0.30f), t);
    }
    if (timeOfDay >= 0.28f && timeOfDay < 0.72f) {
        return glm::vec3(0.90f, 0.93f, 0.97f);
    }
    return glm::vec3(0.09f, 0.11f, 0.16f);
}

float DayNightCycle::GetFogDensity() const {
    if (timeOfDay >= 0.14f && timeOfDay < 0.28f) {
        const float t = InvLerp(0.14f, 0.28f, timeOfDay);
        return glm::mix(0.0036f, 0.0013f, t);
    }
    if (timeOfDay >= 0.28f && timeOfDay < 0.72f) {
        return 0.0010f;
    }
    if (timeOfDay >= 0.72f && timeOfDay < 0.88f) {
        const float t = InvLerp(0.72f, 0.88f, timeOfDay);
        return glm::mix(0.0020f, 0.0048f, t);
    }
    return 0.0058f;
}

void DayNightCycle::Update(float dt) {
    if (!autoAdvance || cycleDurationSeconds <= 0.0f) {
        return;
    }

    loopPhaseElapsed += dt;

    const float currentPhaseDuration = cycleDurationSeconds * GetLoopPhaseWeight(loopPhase);
    if (currentPhaseDuration <= 0.0f) {
        return;
    }

    while (loopPhaseElapsed >= currentPhaseDuration) {
        loopPhaseElapsed -= currentPhaseDuration;
        loopPhase = AdvanceLoopPhase(loopPhase);
        ApplyLoopPhasePreset(loopPhase, timeOfDay);
    }
}
