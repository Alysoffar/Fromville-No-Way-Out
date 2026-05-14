#pragma once

#include <string>
#include <vector>
#include <array>

#include "game/puzzles/core/PuzzleBase.h"

class TextRenderer;

class TabithaWhisperRoutePuzzle final : public PuzzleBase {
public:
    TabithaWhisperRoutePuzzle(std::string title, std::string clue, std::string solvedMessage);

    void Start() override;
    void Update(float dt, const InputContext& input) override;
    void Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const override;
    bool IsSolved() const override { return solved; }
    std::string GetTitle() const override { return "The Whisper Route"; }
    std::string GetClueText() const override { return clue; }
    std::string GetSolvedMessage() const override { return solvedMessage; }
    std::string SerializeState() const override;
    void DeserializeState(const std::string& state) override;
    void Reset() override;
    int GetSelectedIndex() const override { return selectedInterpretation >= 0 ? selectedInterpretation + 1 : -1; }

private:
    enum class Phase {
        InitialContact = 0,
        PatternRecognition,
        EnvironmentalInvestigation,
        EmotionalUnderstanding,
        Completed
    };

    struct Node {
        std::string locationDesc;
        std::string whisperBase;     // initial whisper text
        std::string whisperDetail;   // richer detail when listening
        std::string ambientCue;      // tapping/crying/descriptors
        int hazardLevel; // 0 = safe, 1 = wary, 2 = dangerous
        int correctInterpretation; // 0..3
        std::array<std::string,4> options;
        bool visited = false;
        bool evidenceCollected = false;
        bool revealed = false;
    };

    std::string title;
    std::string clue;
    std::string solvedMessage;

    std::vector<Node> nodes;
    Phase phase = Phase::InitialContact;
    int currentNode = 0;
    int selectedInterpretation = -1;
    float ambientTension = 0.0f;
    bool solved = false;
    bool failed = false;
    std::string statusLine;
    float statusTimer = 0.0f;

    // investigation journal
    std::vector<std::string> journal;
    bool journalVisible = false;

    // counters for progression
    int evidenceCount = 0;

    void ApplyInterpretation(int choice);
    void InspectNode();
    void ListenNode();
    void MoveNode(int delta);
    void AddJournalEntry(const std::string& entry);
    void TryAdvancePhase();
};
