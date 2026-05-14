#pragma once

#include <array>
#include <string>

#include "game/puzzles/core/PuzzleBase.h"

class TextRenderer;

class VictorMirrorCollapsePuzzle final : public PuzzleBase {
public:
    VictorMirrorCollapsePuzzle(std::string title, std::string clue, std::string solvedMessage);

    void Start() override;
    void Update(float dt, const InputContext& input) override;
    void Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const override;
    bool IsSolved() const override { return solved; }
    std::string GetTitle() const override { return "Mirror Memory Collapse"; }
    std::string GetClueText() const override { return clue; }
    std::string GetSolvedMessage() const override { return solvedMessage; }
    std::string SerializeState() const override;
    void DeserializeState(const std::string& state) override;
    void Reset() override;
    bool IsCorrupted() const override { return falseLoopDepth >= 3; }
    int GetSelectedIndex() const override { return selectedMirror + 1; }

private:
    std::string title;
    std::string clue;
    std::string solvedMessage;

    std::array<std::string, 4> mirrors = {{
        "Hall mirror: Victor is older, but the wallpaper is from before the massacre.",
        "Bedroom mirror: a woman screams with no reflection behind her.",
        "Kitchen mirror: the blood is on the ceiling, matching the real room stain.",
        "Stair mirror: a child waves from a room that has no door."
    }};
    std::array<std::string, 4> physicalClues = {{
        "Real wall: faded flower wallpaper under fresh paint.",
        "Real floor: no broken glass, but a scream shakes the frame.",
        "Real ceiling: a dark stain spreads when Victor looks away.",
        "Real hallway: a boarded rectangle where a door should be."
    }};
    std::array<bool, 4> trusted = {{false, false, false, false}};
    std::array<bool, 4> decided = {{false, false, false, false}};
    std::array<bool, 4> realMemory = {{true, false, true, true}};

    int selectedMirror = 0;
    int truthRecovered = 0;
    int falseLoopDepth = 0;
    int layoutState = 0;
    bool solved = false;
    float flickerTimer = 0.0f;
    float sanityDecay = 0.0f;
    float statusTimer = 0.0f;
    std::string statusLine;

    void ShiftHouse();
    void InspectMirror(int index);
    void TrustSelected(bool trust);
    int DecisionsMade() const;
};
