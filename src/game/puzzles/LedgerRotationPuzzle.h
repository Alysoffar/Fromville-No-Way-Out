#pragma once

#include <array>
#include <string>

#include <glm/glm.hpp>

#include "game/puzzles/PuzzleBase.h"

class LedgerRotationPuzzle final : public PuzzleBase {
public:
    LedgerRotationPuzzle(std::string title, std::string clue, std::string solvedMessage);

    void Start() override;
    void Update(float dt, const InputManager& input) override;
    void Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const override;
    bool IsSolved() const override { return solved; }
    std::string GetTitle() const override { return title; }
    std::string GetClueText() const override { return clue; }
    std::string GetSolvedMessage() const override { return solvedMessage; }
    std::string SerializeState() const override;
    void DeserializeState(const std::string& state) override;
    void Reset() override;
    int GetSelectedIndex() const override { return selectedNode + 1; }

private:
    std::string title;
    std::string clue;
    std::string solvedMessage;
    std::array<int, 3> currentRotation{0, 0, 0};
    std::array<int, 3> targetRotation{1, 3, 2};
    std::array<glm::mat4, 3> nodeTransforms{};
    int selectedNode = 0;
    bool solved = false;

    void UpdateTransforms();
    bool MatchesTarget() const;
    static int NormalizeRotation(int rotation);
};