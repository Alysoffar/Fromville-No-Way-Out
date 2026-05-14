#pragma once

#include <array>
#include <string>

#include <glm/glm.hpp>

#include "game/puzzles/core/PuzzleBase.h"

class EvidenceBoardPuzzle final : public PuzzleBase {
public:
    EvidenceBoardPuzzle(std::string title, std::string clue, std::string solvedMessage);

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
    int GetSelectedIndex() const override { return selectedItem + 1; }

private:
    std::string title;
    std::string clue;
    std::string solvedMessage;
    std::array<glm::ivec2, 3> itemPoints = { glm::ivec2(0, 0), glm::ivec2(2, 1), glm::ivec2(-1, 3) };
    int selectedItem = 0;
    bool solved = false;
    float cultTerritoryArea = 6.0f;
    float tolerance = 0.35f;

    static float TriangleArea(const glm::ivec2& a, const glm::ivec2& b, const glm::ivec2& c);
    bool IsSolvedByGeometry() const;
    void ClampItem(glm::ivec2& item) const;
};