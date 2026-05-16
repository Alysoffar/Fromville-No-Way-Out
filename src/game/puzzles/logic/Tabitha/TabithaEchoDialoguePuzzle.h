#pragma once

#include <string>

#include "game/puzzles/core/PuzzleBase.h"

class TextRenderer;

class TabithaEchoDialoguePuzzle final : public PuzzleBase {
public:
    TabithaEchoDialoguePuzzle(std::string title, std::string clue, std::string solvedMessage);

    void Start() override;
    void Update(float dt, const InputContext& input) override;
    void Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const override;
    bool IsSolved() const override { return solved; }
    std::string GetTitle() const override { return "Echo Conversation"; }
    std::string GetClueText() const override { return clue; }
    std::string GetSolvedMessage() const override { return solvedMessage; }
    std::string SerializeState() const override;
    void DeserializeState(const std::string& state) override;
    void Reset() override;

private:
    std::string title;
    std::string clue;
    std::string solvedMessage;

    int step = 0;
    int selectedChoice = 0;
    bool solved = false;

    std::string lineA;
    std::string lineB;
    float lineTimer = 0.0f;
};
