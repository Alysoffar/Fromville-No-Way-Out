#pragma once

#include <string>

#include "game/puzzles/core/PuzzleBase.h"

class TextRenderer;

class SaraMercyTestPuzzle final : public PuzzleBase {
public:
    SaraMercyTestPuzzle(std::string title, std::string clue, std::string solvedMessage);

    void Start() override;
    void Update(float dt, const InputContext& input) override;
    void Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const override;
    bool IsSolved() const override { return finished; }
    std::string GetTitle() const override { return "The Mercy Test"; }
    std::string GetClueText() const override { return clue; }
    std::string GetSolvedMessage() const override { return solvedMessage; }
    std::string SerializeState() const override;
    void DeserializeState(const std::string& state) override;
    void Reset() override;

private:
    std::string title;
    std::string clue;
    std::string solvedMessage;

    int selectedAction = 0; // 0=present evidence,1=empathy,2=threat,3=question
    float pressure = 0.0f; // how much emotional pressure Sara applies
    float prisonerStress = 0.0f; // how prisoner reacts
    float silenceTimer = 0.0f;
    bool finished = false;
    bool verdictMercy = false;

    std::string reactionLine;
    float reactionTimer = 0.0f;
};
