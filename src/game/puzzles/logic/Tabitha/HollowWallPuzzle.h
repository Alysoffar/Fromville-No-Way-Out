#pragma once

#include <array>
#include <string>

#include "game/puzzles/core/PuzzleBase.h"

class TextRenderer;

class HollowWallPuzzle final : public PuzzleBase {
public:
    HollowWallPuzzle(std::string title, std::string clue, std::string solvedMessage);

    void Start() override;
    void Update(float dt, const InputContext& input) override;
    void Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const override;
    bool IsSolved() const override { return solved; }
    std::string GetTitle() const override { return "Children's Word Guess"; }
    std::string GetClueText() const override { return clue; }
    std::string GetSolvedMessage() const override { return solvedMessage; }
    std::string SerializeState() const override;
    void DeserializeState(const std::string& state) override;
    void Reset() override;

private:
    std::string title;
    std::string clue;
    std::string solvedMessage;

    std::string childrenWord = "ankouhi"; // what the children chant
    std::array<std::string, 4> options = {{"HOME", "RIVER", "LIGHT", "DOOR"}};
    int answerIndex = 0; // index in options that is correct

    int selectedOption = 0;
    int guessesLeft = 3;
    bool solved = false;
    bool success = false;

    std::string hintLine;
    float hintTimer = 0.0f;
    std::string statusLine;
    float statusTimer = 0.0f;

    void EvaluateGuess(int index);
    void DropNextHint();
};