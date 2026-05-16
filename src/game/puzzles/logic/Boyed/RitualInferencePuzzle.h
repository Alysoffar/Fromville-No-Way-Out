#pragma once

#include <array>
#include <string>

#include "game/puzzles/core/PuzzleBase.h"

class RitualInferencePuzzle final : public PuzzleBase {
public:
    RitualInferencePuzzle(std::string title, std::string clue, std::string solvedMessage);

    void Start() override;
    void Update(float dt, const InputContext& input) override;
    void Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const override;
    bool IsSolved() const override { return solved; }
    std::string GetTitle() const override { return title; }
    std::string GetClueText() const override { return clue; }
    std::string GetSolvedMessage() const override { return solvedMessage; }
    std::string SerializeState() const override;
    void DeserializeState(const std::string& state) override;
    void Reset() override;
    int GetSelectedIndex() const override { return selectedBrazier + 1; }

private:
    std::string title;
    std::string clue;
    std::string solvedMessage;

    std::array<std::string, 3> braziers{"NORTH", "EAST", "WEST"};
    std::array<std::string, 3> sigils{"ROOT", "MIRROR", "THREAD"};
    std::array<int, 3> assignment{2, 0, 1};
    std::array<int, 3> target{0, 1, 2};

    int selectedBrazier = 0;
    int cluesSatisfied = 0;
    bool solved = false;
    std::string statusLine;
    float statusTimer = 0.0f;

    int EvaluateClues() const;
    void CheckSolution();
};
