#include "DayNightCycle.h"

DayNightCycle& DayNightCycle::Get() {
    static DayNightCycle instance;
    return instance;
}

bool DayNightCycle::IsDaytime() const {
    return timeOfDay >= 6.0f && timeOfDay <= 18.0f;
}

void DayNightCycle::SetTime(float t) {
    timeOfDay = t;
}

float DayNightCycle::GetTimeOfDay() const {
    return timeOfDay;
}

void DayNightCycle::Update(float dt) {
    timeOfDay += dt * 0.01f;
    if (timeOfDay >= 24.0f) timeOfDay -= 24.0f;
}
