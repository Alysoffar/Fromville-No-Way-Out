#pragma once

#include <string>

#include "game/puzzles/core/PuzzleBase.h"

class TextRenderer;

class SaraBreathingClosetPuzzle final : public PuzzleBase {
public:
    SaraBreathingClosetPuzzle(std::string title, std::string clue, std::string solvedMessage);

    void Start() override;
    void Update(float dt, const InputContext& input) override;
    void Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const override;
    bool IsSolved() const override { return solved; }
    std::string GetTitle() const override { return "The Breathing Closet"; }
    std::string GetClueText() const override { return clue; }
    std::string GetSolvedMessage() const override { return solvedMessage; }
    std::string SerializeState() const override;
    void DeserializeState(const std::string& state) override;
    void Reset() override;

private:
    std::string title;
    std::string clue;
    std::string solvedMessage;

    float hideTimer = 0.0f; // how long to survive
    float eventTimer = 0.0f;
    float panic = 0.0f; // 0..1
    int selectedChoice = 0;
    bool solved = false;

    std::string statusLine;
    float statusTimer = 0.0f;

    void TriggerSoundEvent();
};
