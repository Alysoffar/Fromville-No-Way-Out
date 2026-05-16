#pragma once

#include <array>
#include <string>

#include "game/puzzles/core/PuzzleBase.h"
#include "game/memory/MemoryReplaySystem.h"

class TextRenderer;

class GhostWitnessPuzzle final : public PuzzleBase {
public:
    GhostWitnessPuzzle(std::string title, std::string clue, std::string solvedMessage);

    void Start() override;
    void Update(float dt, const InputContext& input) override;
    void Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const override;
    bool IsSolved() const override { return solved; }
    std::string GetTitle() const override { return "Ghost Witness Interrogation"; }
    std::string GetClueText() const override { return clue; }
    std::string GetSolvedMessage() const override { return solvedMessage; }
    std::string SerializeState() const override;
    void DeserializeState(const std::string& state) override;
    void Reset() override;

private:
    struct Witness {
        std::string name;
        std::array<std::string, 2> lines;
        int contradictionScore = 0;
    };

    std::string title;
    std::string clue;
    std::string solvedMessage;

    std::array<Witness, 4> witnesses = {{
        {"Ella", {"He never came back from the cellar.", "I saw him leave with the key."}, 1},
        {"Thomas", {"The candles were already lit.", "I lit them myself."}, 2},
        {"Mara", {"There were only two of us.", "Everyone was counting three."}, 3},
        {"The usher", {"I was dead before the bell.", "I rang the bell twice."}, 4}
    }};

    int selectedWitness = 0;
    int successfulInterrogations = 0;
    int falseAccusations = 0;
    bool solved = false;

    float ghostPulse = 0.0f;
    float instability = 0.0f;
    int lineVariant = 0;
    std::string statusLine;
    float statusTimer = 0.0f;

    void ToggleLineVariant();
    void AccuseWitness(int index);
};