#include "game/story/StoryManager.h"

#include <algorithm>

StoryManager& StoryManager::Instance() {
    static StoryManager instance;
    return instance;
}

void StoryManager::Reset() {
    storyStep = StoryStep::Investigation;
    globalTension = 0.15f;
    hintTimer = 0.0f;
    crypticHintIndex = 0;
}

void StoryManager::AdvanceStep(int objectiveIndex) {
    switch (objectiveIndex) {
        case 0: storyStep = StoryStep::Investigation; break;
        case 1: storyStep = StoryStep::Evidence; break;
        case 2: storyStep = StoryStep::Confrontation; break;
        default: storyStep = StoryStep::Ritual; break;
    }

    const float tensionBase = 0.18f + static_cast<float>(objectiveIndex) * 0.18f;
    SetGlobalTension(std::clamp(tensionBase, 0.0f, 1.0f));
    ResetHintTimer();
}

void StoryManager::Update(float dt) {
    globalTension = std::clamp(globalTension + dt * 0.01f, 0.0f, 1.0f);
    if (hintTimer > 0.0f) {
        hintTimer = std::max(0.0f, hintTimer - dt * 0.5f);
    }
}

void StoryManager::SetGlobalTension(float value) {
    globalTension = std::clamp(value, 0.0f, 1.0f);
}

void StoryManager::ResetHintTimer() {
    hintTimer = 0.0f;
    crypticHintIndex = (crypticHintIndex + 1) % crypticLore.size();
}

void StoryManager::AdvanceHintTimer(float dt) {
    hintTimer += dt;
}

bool StoryManager::ShouldDecayHintText() const {
    return hintTimer >= 10.0f;
}

std::string StoryManager::GetCrypticHintForObjective(int objectiveIndex, const std::string& objectiveDescription) const {
    if (objectiveIndex == 1) {
        return "The ledger listens when each piece turns the same way.";
    }
    if (objectiveIndex == 2) {
        return "Draw the triangle until its shadow matches the cult's footprint.";
    }
    if (objectiveIndex == 3) {
        return "Stand where the witness cannot see the knife behind your back.";
    }
    if (objectiveIndex >= 4) {
        return "Hold the circle while the dark pushes back.";
    }

    if (!objectiveDescription.empty()) {
        return crypticLore[crypticHintIndex] + " " + objectiveDescription;
    }
    return crypticLore[crypticHintIndex];
}
