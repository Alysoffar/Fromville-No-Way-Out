#pragma once

#include <array>
#include <string>

#include "game/puzzles/core/PuzzleBase.h"

class CultDebatePuzzle final : public PuzzleBase {
public:
    CultDebatePuzzle(std::string title, std::string clue, std::string solvedMessage);

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
    int GetSelectedIndex() const override { return selectedOption + 1; }

private:
    struct DebateRound {
        std::string accusation;
        std::array<std::string, 3> responses;
        int winningResponse = 0;
    };

    std::string title;
    std::string clue;
    std::string solvedMessage;
    std::array<DebateRound, 3> rounds;

    int roundIndex = 0;
    int selectedOption = 0;
    int strongResponses = 0;
    bool solved = false;
    bool failed = false;
    std::string statusLine;
    float statusTimer = 0.0f;

    void SubmitChoice();
};
