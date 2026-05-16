#pragma once

#include <array>
#include <string>
#include "game/puzzles/core/PuzzleBase.h"

class TextRenderer;

class VictorMemoryTimelinePuzzle final : public PuzzleBase {
public:
    VictorMemoryTimelinePuzzle(std::string title, std::string clue, std::string solvedMessage);

    void Start() override;
    void Update(float dt, const InputContext& input) override;
    void Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const override;
    bool IsSolved() const override { return solved; }
    std::string GetTitle() const override { return "Who Is Missing? Puzzle"; }
    std::string GetClueText() const override { return clue; }
    std::string GetSolvedMessage() const override { return solvedMessage; }
    std::string SerializeState() const override;
    void DeserializeState(const std::string& state) override;
    void Reset() override;

private:
    std::string title;
    std::string clue;
    std::string solvedMessage;

    std::array<std::string, 4> people = {{
        "Alyssa Reed",
        "Victor Hale",
        "Mara Stone",
        "The unnamed child"
    }};
    std::array<std::string, 4> clues = {{
        "Left a spoon at the table",
        "Wore a red scarf",
        "Kept the chapel keys",
        "No one remembers their face"
    }};

    int missingIndex = 0;
    int successfulFinds = 0;
    int wrongGuesses = 0;
    bool solved = false;

    float timer = 0.0f;
    float absencePressure = 0.0f;
    std::string statusLine;
    float statusTimer = 0.0f;

    void ShiftMissingPerson();
    void GuessMissing(int index);
};
