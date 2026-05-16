#pragma once

#include <array>
#include <string>
#include "game/puzzles/core/PuzzleBase.h"

class TextRenderer;

class TabithaTunnelPuzzle final : public PuzzleBase {
public:
    TabithaTunnelPuzzle(std::string title, std::string clue, std::string solvedMessage);

    void Start() override;
    void Update(float dt, const InputContext& input) override;
    void Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const override;
    bool IsSolved() const override { return solved; }
    std::string GetTitle() const override { return "Echo Mimic Puzzle"; }
    std::string GetClueText() const override { return clue; }
    std::string GetSolvedMessage() const override { return solvedMessage; }
    std::string SerializeState() const override;
    void DeserializeState(const std::string& state) override;
    void Reset() override;

private:
    struct VoiceSample {
        std::string speaker;
        std::string line;
        std::string breathPattern;
        bool mimic = false;
    };

    std::string title;
    std::string clue;
    std::string solvedMessage;

    std::array<VoiceSample, 4> samples = {{
        {"Alyssa", "I'm at the gate. Hurry.", "short, short, long", false},
        {"Mara", "Please don't leave me here.", "no inhale before speaking", true},
        {"Victor", "I never came down here.", "steady, steady, steady", false},
        {"Mimic", "I found you already.", "off-beat, flat, silent", true}
    }};
    std::array<bool, 4> identified = {{false, false, false, false}};

    int currentSample = 0;
    int selectedSuspect = 0;
    int correctIdentifications = 0;
    int falseCalls = 0;
    bool solved = false;

    float sampleTimer = 0.0f;
    float paranoia = 0.0f;
    std::string statusLine;
    float statusTimer = 0.0f;

    void AdvanceSample();
    void EvaluateSuspect(int index);
    int MimicCount() const;
};
