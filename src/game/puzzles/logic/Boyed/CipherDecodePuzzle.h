#pragma once

#include <array>
#include <string>

#include "game/puzzles/core/PuzzleBase.h"

class CipherDecodePuzzle final : public PuzzleBase {
public:
    CipherDecodePuzzle(std::string title, std::string clue, std::string encoded, std::array<std::string, 4> options, int correctIndex, std::string solvedMessage);

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

private:
    std::string title;
    std::string clue;
    std::string encoded;
    std::array<std::string, 4> options;
    int correctIndex = 0;
    std::string solvedMessage;
    bool solved = false;
    int selectedIndex = -1;
};