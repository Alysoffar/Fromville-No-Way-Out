#pragma once

#include <array>
#include <string>

#include "game/moral/MoralCorruptionSystem.h"
#include "game/puzzles/core/AtmosphereSystem.h"
#include "game/puzzles/core/ContradictionTracker.h"
#include "game/puzzles/core/HallucinationOverlay.h"
#include "game/puzzles/core/PuzzleBase.h"
#include "game/puzzles/core/SanitySystem.h"
#include "game/puzzles/core/TruthSystem.h"

class TextRenderer;

class SaraFinalJudgmentPuzzle final : public PuzzleBase {
public:
    SaraFinalJudgmentPuzzle(std::string title, std::string clue, std::string solvedMessage);

    void Start() override;
    void Update(float dt, const InputContext& input) override;
    void Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const override;
    bool IsSolved() const override { return solved; }
    std::string GetTitle() const override { return "Final Psychological Confrontation"; }
    std::string GetClueText() const override { return clue; }
    std::string GetSolvedMessage() const override { return solvedMessage; }
    std::string SerializeState() const override;
    void DeserializeState(const std::string& state) override;
    void Reset() override;

private:
    enum class Phase {
        Investigation,
        CrossExamination,
        Collapse,
        Verdict
    };

    std::string title;
    std::string clue;
    std::string solvedMessage;

    std::array<std::string, 4> evidence = {{
        "The door was locked.",
        "The witness heard two voices.",
        "The victim's handwriting changed.",
        "The chapel bell rang twice."
    }};
    int evidenceIndex = 0;
    int evidenceExposed = 0;
    int wrongPressures = 0;
    Phase phase = Phase::Investigation;
    MoralChoice finalChoice = MoralChoice::Exposure;
    bool choiceMade = false;
    bool solved = false;

    float timer = 0.0f;
    float pressure = 0.0f;
    std::string statusLine;
    float statusTimer = 0.0f;

    SanitySystem sanity;
    TruthSystem truth;
    ContradictionTracker tracker;
    HallucinationOverlay overlay;
    AtmosphereSystem atmosphere;

    void ExposeEvidence(int index);
    void AdvancePhase();
    void CommitFinalChoice(MoralChoice choice);
};