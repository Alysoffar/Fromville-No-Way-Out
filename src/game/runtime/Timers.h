#pragma once

#include <algorithm>

class DurationTimer {
public:
    DurationTimer() = default;
    explicit DurationTimer(float seconds) : remaining(std::max(0.0f, seconds)) {}

    void Start(float seconds) {
        remaining = std::max(0.0f, seconds);
    }

    void Stop() {
        remaining = 0.0f;
    }

    void Tick(float dt) {
        remaining = std::max(0.0f, remaining - dt);
    }

    float Remaining() const {
        return remaining;
    }

    bool IsActive() const {
        return remaining > 0.0f;
    }

private:
    float remaining = 0.0f;
};

class CooldownTimer {
public:
    CooldownTimer() = default;
    explicit CooldownTimer(float seconds) : remaining(std::max(0.0f, seconds)) {}

    void Trigger(float seconds) {
        remaining = std::max(0.0f, seconds);
    }

    void Tick(float dt) {
        remaining = std::max(0.0f, remaining - dt);
    }

    bool IsReady() const {
        return remaining <= 0.0f;
    }

    float Remaining() const {
        return remaining;
    }

private:
    float remaining = 0.0f;
};
