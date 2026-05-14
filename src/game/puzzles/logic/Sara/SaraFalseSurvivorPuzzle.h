#pragma once

#include <string>

#include "game/puzzles/core/PuzzleBase.h"

class TextRenderer;

class SaraFalseSurvivorPuzzle final : public PuzzleBase {
public:
    SaraFalseSurvivorPuzzle(std::string title, std::string clue, std::string solvedMessage);

    void Start() override;
    void Update(float dt, const InputContext& input) override;
    void Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const override;
    bool IsSolved() const override { return concluded; }
    std::string GetTitle() const override { return "The False Survivor"; }
    std::string GetClueText() const override { return clue; }
    std::string GetSolvedMessage() const override { return solvedMessage; }
    std::string SerializeState() const override;
    void DeserializeState(const std::string& state) override;
    void Reset() override;

private:
    std::string title;
    std::string clue;
    std::string solvedMessage;

    int selectedProbe = 0;
    int probeCount = 0;
    float suspicion = 0.0f; // 0..1 inferred from cues
    bool concluded = false;

    std::string survivorLine;
    std::string observationLine;
    float lineTimer = 0.0f;

    void AdvanceProbe(int probe);
};
