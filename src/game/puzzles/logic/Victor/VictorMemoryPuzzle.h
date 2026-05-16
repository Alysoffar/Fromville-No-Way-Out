#pragma once

#include <array>
#include <string>

#include "game/puzzles/core/PuzzleBase.h"
#include "game/memory/MemoryReplaySystem.h"

class TextRenderer;

class VictorMemoryPuzzle final : public PuzzleBase {
public:
    VictorMemoryPuzzle(std::string title, std::string clue, std::string solvedMessage);

    void Start() override;
    void Update(float dt, const InputContext& input) override;
    void Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const override;
    bool IsSolved() const override { return solved; }
    std::string GetTitle() const override { return "Memory Corruption Puzzle"; }
    std::string GetClueText() const override { return clue; }
    std::string GetSolvedMessage() const override { return solvedMessage; }
    std::string SerializeState() const override;
    void DeserializeState(const std::string& state) override;
    void Reset() override;

private:
    std::string title;
    std::string clue;
    std::string solvedMessage;

    std::array<std::string, 4> scenes = {{
        "Basement door",
        "Kitchen table",
        "Hallway mirror",
        "Missing child room"
    }};
    int corruptedScene = 0;
    int detectedCorruptions = 0;
    int wrongCalls = 0;
    bool solved = false;

    float loopTimer = 0.0f;
    float trauma = 0.0f;
    std::string statusLine;
    float statusTimer = 0.0f;

    void CorruptLoop();
    void InspectScene(int index);
};