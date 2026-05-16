#include "game/puzzles/logic/Tabitha/TabithaEchoDialoguePuzzle.h"

#include <sstream>

#include "engine/renderer/TextRenderer.h"

TabithaEchoDialoguePuzzle::TabithaEchoDialoguePuzzle(std::string title, std::string clue, std::string solvedMessage)
    : title(std::move(title)), clue(std::move(clue)), solvedMessage(std::move(solvedMessage)) {}

void TabithaEchoDialoguePuzzle::Start() {
    Reset();
    lineA = "A child's laugh echoes, then a whisper: 'ankouhi'.";
    lineB = "Tabitha listens, eyes unfocused; her fingers trace the seam of the map.";
    lineTimer = 3.0f;
    PlaySound("puzzle_open");
}

void TabithaEchoDialoguePuzzle::Update(float dt, const InputContext& input) {
    if (input.IsActionPressed(InputAction::PuzzleReset)) { Start(); return; }
    if (solved) return;

    lineTimer = std::max(0.0f, lineTimer - dt);

    if (input.IsActionPressed(InputAction::PuzzleOption1)) selectedChoice = 0;
    if (input.IsActionPressed(InputAction::PuzzleOption2)) selectedChoice = 1;
    if (input.IsActionPressed(InputAction::PuzzleOption3)) selectedChoice = 2;
    if (input.IsActionPressed(InputAction::PuzzleOption4)) selectedChoice = 3;

    if (input.IsActionPressed(InputAction::PuzzleConfirm)) {
        // move the conversation forward, choices modify narration
        ++step;
        switch (selectedChoice) {
            case 0: lineA = "You nudge Tabitha to stare at the children. She swallows and nods."; break;
            case 1: lineA = "You suggest listening for rhythm; Tabitha tilts her head as if counting breaths."; break;
            case 2: lineA = "You press for meaning; Tabitha's jaw tightens, the smile drops from her face."; break;
            case 3: lineA = "You stay silent; the echo fills the space and her pupils dilate in the dark."; break;
        }
        lineB = "The children repeat the syllable. Something in Tabitha's posture changes.";
        lineTimer = 3.0f;
        PlaySound("conversation_tick");
    }

    if (step >= 3) {
        solved = true;
        solvedMessage = "Tabitha understands the cadence; the map shifts in her mind.";
        PlaySound("puzzle_complete");
    }
}

void TabithaEchoDialoguePuzzle::Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const {
    const float topY = static_cast<float>(screenHeight) * 0.18f;
    const glm::vec3 titleColor(0.7f, 0.95f, 0.9f);
    textRenderer.RenderText(GetTitle(), 72.0f, topY, 0.72f, titleColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(title, 72.0f, topY + 34.0f, 0.34f, glm::vec3(0.9f) * alpha, screenWidth, screenHeight);
    if (!lineA.empty()) textRenderer.RenderText(lineA, 72.0f, topY + 86.0f, 0.34f, glm::vec3(0.92f) * alpha, screenWidth, screenHeight);
    if (!lineB.empty()) textRenderer.RenderText(lineB, 72.0f, topY + 122.0f, 0.32f, glm::vec3(0.85f) * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("[1-4] pick tone  [ENTER] continue", 72.0f, static_cast<float>(screenHeight) - 72.0f, 0.32f, glm::vec3(0.9f) * alpha, screenWidth, screenHeight);
    if (solved) textRenderer.RenderText(solvedMessage, 72.0f, static_cast<float>(screenHeight) * 0.78f, 0.42f, glm::vec3(0.8f,0.95f,0.9f) * alpha, screenWidth, screenHeight);
}

std::string TabithaEchoDialoguePuzzle::SerializeState() const {
    std::ostringstream s;
    s << step << ';' << selectedChoice << ';' << (solved ? 1 : 0);
    return s.str();
}

void TabithaEchoDialoguePuzzle::DeserializeState(const std::string& state) {
    std::stringstream ss(state);
    std::string tok;
    std::getline(ss, tok, ';'); step = tok.empty() ? 0 : std::stoi(tok);
    std::getline(ss, tok, ';'); selectedChoice = tok.empty() ? 0 : std::stoi(tok);
    std::getline(ss, tok, ';'); solved = (tok == "1");
}

void TabithaEchoDialoguePuzzle::Reset() {
    step = 0;
    selectedChoice = 0;
    solved = false;
    lineA.clear(); lineB.clear(); lineTimer = 0.0f;
}
