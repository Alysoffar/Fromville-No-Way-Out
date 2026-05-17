#pragma once

#include <array>
#include <string>

#include "game/puzzles/core/AudioDistortion.h"
#include "game/puzzles/core/AtmosphereSystem.h"
#include "game/puzzles/core/ContradictionTracker.h"
#include "game/puzzles/core/HallucinationOverlay.h"
#include "game/puzzles/core/PuzzleBase.h"
#include "game/puzzles/core/SanitySystem.h"
#include "game/puzzles/core/TruthSystem.h"

class TextRenderer;

class JadeDebatePuzzle final : public PuzzleBase {
public:
    enum class Phase {
        Opening,
        CrossExamination,
        Pressure,
        Verdict
    };

    enum class Mood {
        Neutral,
        Smug,
        Angry
    };

    struct Choice {
        std::string text;
        std::string angryResponse;
        std::string smugResponse;
        float sanityLoss = 0.0f;
        float truthGain = 0.0f;
    };

    struct Exchange {
        std::string lie;
        std::string truth;
        std::string leadIn;
        int correctChoice = 0;
        std::array<Choice, 4> choices;
    };

    JadeDebatePuzzle() = default;

    void Start() override;
    void Update(float dt, const InputContext& input) override;
    void Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const override;
    bool IsSolved() const override { return solved; }
    bool IsCorrupted() const override { return corrupted; }
    std::string GetTitle() const override { return "Debate With The Man In Yellow"; }
    std::string GetClueText() const override { return "Expose the contradiction buried in each answer."; }
    std::string GetSolvedMessage() const override { return solvedMessage; }
    std::string SerializeState() const override;
    void DeserializeState(const std::string& state) override;
    void Reset() override;

private:
    Phase phase = Phase::Opening;
    int currentPrompt = 0;
    int selectedAnswer = 0;
    int contradictionsExposed = 0;
    int pressureBursts = 0;
    bool active = false;
    bool solved = false;
    bool corrupted = false;
    Mood mood = Mood::Neutral;
    std::string yellowLine;
    std::string responseLine;
    std::string emotionLine;
    std::string solvedMessage = "The Man in Yellow cannot keep the lie alive.";

    float timer = 0.0f;
    float promptTimer = 0.0f;
    float pressureTimer = 0.0f;
    float dialoguePulse = 0.0f;

    SanitySystem sanity;
    TruthSystem truth;
    ContradictionTracker tracker;
    HallucinationOverlay overlay;
    AtmosphereSystem atmosphere;
    AudioDistortion distortion;

    void AdvancePrompt();
    void ExposeContradiction(int index);
    void EscalatePressure(float amount);
};