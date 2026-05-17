#pragma once

#include <array>
#include <string>

#include "game/puzzles/core/PuzzleBase.h"
#include "game/symbols/SymbolSystem.h"

class TextRenderer;

class JadeSymbolVaultPuzzle final : public PuzzleBase {
public:
    JadeSymbolVaultPuzzle(std::string title, std::string clue, std::string solvedMessage);

    void Start() override;
    void Update(float dt, const InputContext& input) override;
    void Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const override;
    bool IsSolved() const override { return solved; }
    std::string GetTitle() const override { return "Living Symbol Infection Puzzle"; }
    std::string GetClueText() const override { return clue; }
    std::string GetSolvedMessage() const override { return solvedMessage; }
    std::string SerializeState() const override;
    void DeserializeState(const std::string& state) override;
    void Reset() override;

private:
    std::string title;
    std::string clue;
    std::string solvedMessage;

    std::array<SymbolType, 4> vaultOrder = {{
        SymbolType::Knowledge,
        SymbolType::Power,
        SymbolType::Truth,
        SymbolType::Awakening
    }};
    std::array<float, 4> infection = {{0.15f, 0.08f, 0.12f, 0.05f}};
    std::array<bool, 4> stabilized = {{false, false, false, false}};

    int activeNode = 0;
    int cleansedCount = 0;
    bool solved = false;

    float timer = 0.0f;
    float spreadTimer = 0.0f;
    float roomDistortion = 0.0f;
    std::string statusLine;
    float statusTimer = 0.0f;

    void StabilizeNode(int index);
    void SpreadInfection();
};