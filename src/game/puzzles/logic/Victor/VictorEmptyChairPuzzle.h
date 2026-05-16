#pragma once

#include <array>
#include <string>

#include "game/puzzles/core/PuzzleBase.h"

class TextRenderer;

class VictorEmptyChairPuzzle final : public PuzzleBase {
public:
    VictorEmptyChairPuzzle(std::string title, std::string clue, std::string solvedMessage);

    void Start() override;
    void Update(float dt, const InputContext& input) override;
    void Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const override;
    bool IsSolved() const override { return solved; }
    std::string GetTitle() const override { return "The Empty Chair"; }
    std::string GetClueText() const override { return clue; }
    std::string GetSolvedMessage() const override { return solvedMessage; }
    std::string SerializeState() const override;
    void DeserializeState(const std::string& state) override;
    void Reset() override;
    bool IsCorrupted() const override { return wrongReconstructions >= 2; }
    int GetSelectedIndex() const override { return selectedIdentity + 1; }

private:
    std::string title;
    std::string clue;
    std::string solvedMessage;

    std::array<std::string, 4> evidence = {{
        "A family photograph: five bodies, four scratched faces, one empty outline.",
        "A place setting rocks gently though no one is sitting there.",
        "A diary page says 'Tell ____ I am sorry' where a name has been torn out.",
        "Footsteps circle the chair, stop, then continue without weight."
    }};
    std::array<std::string, 4> rewrittenEvidence = {{
        "The photograph now shows four people. The fifth shadow stains the frame.",
        "The chair is gone. Dust still rocks where its legs should be.",
        "The diary insists nobody sat there. The ink is wet and afraid.",
        "The footsteps reverse direction and end under Victor's shoes."
    }};
    std::array<bool, 4> observed = {{false, false, false, false}};
    std::array<std::string, 4> identities = {{
        "Clara Matthews, the erased daughter",
        "Father Khatri, misremembered",
        "Christopher, wearing another name",
        "Victor himself as a child"
    }};

    int selectedEvidence = 0;
    int selectedIdentity = 0;
    int rewritePhase = 0;
    int wrongReconstructions = 0;
    bool solved = false;
    float rewriteTimer = 0.0f;
    float instability = 0.0f;
    float statusTimer = 0.0f;
    std::string statusLine;

    int ObservationCount() const;
    void RewriteRoom();
    void Observe(int index);
    void Reconstruct();
};
