#pragma once

#include <array>
#include <string>

#include "game/puzzles/core/PuzzleBase.h"

class TextRenderer;

class VictorSilentProcessionPuzzle final : public PuzzleBase {
public:
    VictorSilentProcessionPuzzle(std::string title, std::string clue, std::string solvedMessage);

    void Start() override;
    void Update(float dt, const InputContext& input) override;
    void Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const override;
    bool IsSolved() const override { return solved; }
    std::string GetTitle() const override { return "The Silent Procession"; }
    std::string GetClueText() const override { return clue; }
    std::string GetSolvedMessage() const override { return solvedMessage; }
    std::string SerializeState() const override;
    void DeserializeState(const std::string& state) override;
    void Reset() override;
    bool IsCorrupted() const override { return insertedIntoProcession; }
    int GetSelectedIndex() const override { return selectedPattern + 1; }

private:
    std::string title;
    std::string clue;
    std::string solvedMessage;

    std::array<std::string, 4> observations = {{
        "The third lantern pauses half a step before every turn.",
        "One dead resident appears as a child, then an old man, then neither.",
        "Footsteps strike a cross-shaped rhythm across the street.",
        "The order of lanterns spells the sacrifice route through town."
    }};
    std::array<bool, 4> decoded = {{false, false, false, false}};
    std::array<int, 4> requiredLoop = {{1, 2, 3, 4}};

    int selectedPattern = 0;
    int currentLoop = 1;
    int suspicion = 0;
    bool noticed = false;
    bool insertedIntoProcession = false;
    bool solved = false;
    float loopTimer = 0.0f;
    float distanceToProcession = 0.62f;
    float darkness = 0.0f;
    float statusTimer = 0.0f;
    std::string statusLine;

    int DecodedCount() const;
    void AdvanceLoop();
    void MarkPattern(int index);
    void Approach(float amount);
};
