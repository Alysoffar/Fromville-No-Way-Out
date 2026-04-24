#pragma once
#include <glm/glm.hpp>

enum class DayPhase {
    Dawn,
    Day,
    Dusk,
    Night,
};

enum class GameLoopPhase {
    Explore,
    Prepare,
    Survive,
    SwitchCharacter,
};

class DayNightCycle {
public:
    static DayNightCycle& Get();
    bool IsDaytime() const;
    void SetTime(float t);
    float GetTimeOfDay() const;
    void SetPhase(DayPhase phase);
    DayPhase GetPhase() const;
    const char* GetPhaseName() const;
    void SetLoopPhase(GameLoopPhase phase);
    GameLoopPhase GetLoopPhase() const;
    void SetAutoAdvance(bool enabled);
    void ToggleAutoAdvance();
    bool IsAutoAdvanceEnabled() const;
    void SetCycleDurationSeconds(float seconds);
    float GetCycleDurationSeconds() const;
    float GetDaylightFactor() const;
    glm::vec3 GetSunDirection() const;
    glm::vec3 GetAmbientColor() const;
    glm::vec3 GetDirectionalLightColor() const;
    glm::vec3 GetFogColor() const;
    float GetFogDensity() const;
    void Update(float dt);

private:
    float timeOfDay = 0.76f;
    float cycleDurationSeconds = 720.0f;
    bool autoAdvance = true;
    GameLoopPhase loopPhase = GameLoopPhase::Explore;
    float loopPhaseElapsed = 0.0f;
};
