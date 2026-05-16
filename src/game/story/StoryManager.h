#pragma once

#include <array>
#include <string>

class StoryManager {
public:
    static StoryManager& Instance();

    enum class StoryStep {
        Investigation,
        Evidence,
        Confrontation,
        Ritual
    };

    void Reset();
    void AdvanceStep(int objectiveIndex);
    void Update(float dt);

    void SetGlobalTension(float value);
    float GetGlobalTension() const { return globalTension; }
    StoryStep GetStoryStep() const { return storyStep; }
    float GetHintTimer() const { return hintTimer; }
    void ResetHintTimer();
    void AdvanceHintTimer(float dt);
    bool ShouldDecayHintText() const;
    std::string GetCrypticHintForObjective(int objectiveIndex, const std::string& objectiveDescription) const;

private:
    StoryManager() = default;

    StoryStep storyStep = StoryStep::Investigation;
    float globalTension = 0.15f;
    float hintTimer = 0.0f;
    std::size_t crypticHintIndex = 0;
    std::array<std::string, 4> crypticLore = {
        "The town points where the soil is warm.",
        "Three truths, one shape, and a door in the dark.",
        "A leader is only a shadow until you stand behind it.",
        "Rituals break when the circle is drawn wrong."
    };
};
