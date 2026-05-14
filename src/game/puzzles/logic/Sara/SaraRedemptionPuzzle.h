#pragma once

#include <array>
#include <string>

#include "game/moral/MoralCorruptionSystem.h"
#include "game/puzzles/core/PuzzleBase.h"

class TextRenderer;

class SaraRedemptionPuzzle final : public PuzzleBase {
public:
    SaraRedemptionPuzzle(std::string title, std::string clue, std::string solvedMessage);

    void Start() override;
    void Update(float dt, const InputContext& input) override;
    void Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const override;
    bool IsSolved() const override { return solved; }
    std::string GetTitle() const override { return "Hidden Confession / Fear Detection"; }
    std::string GetClueText() const override { return clue; }
    std::string GetSolvedMessage() const override { return solvedMessage; }
    std::string SerializeState() const override;
    void DeserializeState(const std::string& state) override;
    void Reset() override;

private:
    enum class Phase {
        Listening,
        Analysis,
        Judgment
    };

    struct Suspect {
        std::string name;
        std::string bodyLanguage;
        int guilt = 0;
    };

    std::string title;
    std::string clue;
    std::string solvedMessage;

    std::array<Suspect, 4> suspects = {{
        {"Alyssa", "hunched shoulders", 1},
        {"Mara", "too still", 3},
        {"Victor", "keeps touching pocket", 2},
        {"The usher", "smiles at the wrong moments", 4}
    }};

    Phase phase = Phase::Listening;
    int selectedSuspect = 0;
    int fragmentsRecovered = 0;
    int fearReadings = 0;
    bool solved = false;

    float noise = 0.0f;
    float calm = 0.0f;
    float timer = 0.0f;
    std::string statusLine;
    float statusTimer = 0.0f;

    void RecoverFragment();
    void AnalyzeSuspect(int index);
    void CommitJudgment(int index);
};