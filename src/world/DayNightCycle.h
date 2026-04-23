#pragma once
#include <glm/glm.hpp>

class DayNightCycle {
public:
    static DayNightCycle& Get();
    bool IsDaytime() const;
    void SetTime(float t);
    float GetTimeOfDay() const;
    glm::vec3 GetSunDirection() const { return glm::vec3(0.0f, -1.0f, 0.0f); }
    void Update(float dt);

private:
    float timeOfDay = 12.0f;
};
