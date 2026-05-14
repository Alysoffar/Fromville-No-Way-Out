#include "game/puzzles/logic/Sara/SaraMercyTestPuzzle.h"

#include <sstream>

#include "engine/renderer/TextRenderer.h"

SaraMercyTestPuzzle::SaraMercyTestPuzzle(std::string title, std::string clue, std::string solvedMessage)
    : title(std::move(title)), clue(std::move(clue)), solvedMessage(std::move(solvedMessage)) {}

void SaraMercyTestPuzzle::Start() {
    Reset();
    reactionLine = "The prisoner sits bound; breath shallow, eyes avoiding yours.";
    reactionTimer = 3.0f;
    PlaySound("puzzle_open");
}

void SaraMercyTestPuzzle::Update(float dt, const InputContext& input) {
    if (input.IsActionPressed(InputAction::PuzzleReset)) { Start(); return; }
    if (finished) return;

    reactionTimer = std::max(0.0f, reactionTimer - dt);

    if (input.IsActionPressed(InputAction::PuzzleOption1)) selectedAction = 0;
    if (input.IsActionPressed(InputAction::PuzzleOption2)) selectedAction = 1;
    if (input.IsActionPressed(InputAction::PuzzleOption3)) selectedAction = 2;
    if (input.IsActionPressed(InputAction::PuzzleOption4)) selectedAction = 3;

    // Silence mechanic: holding PuzzleCancel counts as a held silence (use IsActionDown)
    if (input.IsActionDown(InputAction::PuzzleCancel)) {
        silenceTimer += dt;
        if (silenceTimer > 1.2f && silenceTimer < 3.5f) {
            reactionLine = "Silence fills the room. The prisoner inhales sharply; something slips in their eyes.";
            prisonerStress += 0.12f;
            reactionTimer = 2.2f;
        }
    } else {
        silenceTimer = 0.0f;
    }

    if (input.IsActionPressed(InputAction::PuzzleConfirm)) {
        // apply selected action
        switch (selectedAction) {
            case 0: // present evidence
                pressure += 0.25f;
                reactionLine = "You lay the photograph between you. Their hands tremble and a lie fractures.";
                prisonerStress += 0.18f;
                PlaySound("paper_slide");
                break;
            case 1: // empathy
                pressure += 0.06f;
                reactionLine = "Your voice softens; the prisoner blinks and something like relief flickers.";
                prisonerStress += 0.06f;
                PlaySound("soft_sigh");
                break;
            case 2: // threat
                pressure += 0.35f;
                reactionLine = "You lean forward. They flinch; a vein pulses at their temple.";
                prisonerStress += 0.28f;
                PlaySound("threat_note");
                break;
            case 3: // question
                pressure += 0.12f;
                reactionLine = "A simple question; they answer, but the voice cracks.";
                prisonerStress += 0.1f;
                PlaySound("question_ping");
                break;
        }
        reactionTimer = 2.4f;
    }

    // natural decay/response
    prisonerStress = std::min(1.0f, prisonerStress);
    pressure = std::min(1.0f, pressure);

    // outcomes
    if (prisonerStress >= 0.95f) {
        finished = true;
        verdictMercy = false;
        solvedMessage = "The prisoner breaks completely — truth is messy, and your hands are stained.";
        reactionLine = "They sob, then whisper a fractured truth. You are left with the weight of what you did.";
        PlaySound("breakdown");
    } else if (pressure >= 0.9f) {
        finished = true;
        verdictMercy = true;
        solvedMessage = "Under pressure they confess enough for mercy to feel like justice.";
        reactionLine = "They close their eyes and speaks a truth that smells like both regret and relief.";
        PlaySound("confession");
    }
}

void SaraMercyTestPuzzle::Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const {
    const float topY = static_cast<float>(screenHeight) * 0.18f;
    const glm::vec3 titleColor(0.92f, 0.9f, 0.88f);

    textRenderer.RenderText(GetTitle(), 72.0f, topY, 0.72f, titleColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(title, 72.0f, topY + 34.0f, 0.34f, glm::vec3(0.86f) * alpha, screenWidth, screenHeight);

    if (!reactionLine.empty())
        textRenderer.RenderText(reactionLine, 72.0f, topY + 86.0f, 0.34f, glm::vec3(0.9f) * alpha, screenWidth, screenHeight);

    textRenderer.RenderText("[1] Present evidence  [2] Empathize  [3] Threaten  [4] Ask direct question", 72.0f, static_cast<float>(screenHeight) - 72.0f, 0.34f, glm::vec3(0.9f) * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("Hold [ESC] to remain silent and wait for a reaction.", 72.0f, static_cast<float>(screenHeight) - 102.0f, 0.30f, glm::vec3(0.8f) * alpha, screenWidth, screenHeight);

    if (finished) {
        textRenderer.RenderText(solvedMessage, 72.0f, static_cast<float>(screenHeight) * 0.78f, 0.42f, glm::vec3(0.9f,0.8f,0.78f) * alpha, screenWidth, screenHeight);
    }
}

std::string SaraMercyTestPuzzle::SerializeState() const {
    std::ostringstream stream;
    stream << (finished ? 1 : 0) << ';' << selectedAction << ';' << pressure << ';' << prisonerStress << ';' << silenceTimer;
    return stream.str();
}

void SaraMercyTestPuzzle::DeserializeState(const std::string& state) {
    std::stringstream ss(state);
    std::string token;
    std::getline(ss, token, ';'); finished = (token == "1");
    std::getline(ss, token, ';'); selectedAction = token.empty() ? 0 : std::stoi(token);
    std::getline(ss, token, ';'); pressure = token.empty() ? 0.0f : std::stof(token);
    std::getline(ss, token, ';'); prisonerStress = token.empty() ? 0.0f : std::stof(token);
    std::getline(ss, token, ';'); silenceTimer = token.empty() ? 0.0f : std::stof(token);
}

void SaraMercyTestPuzzle::Reset() {
    selectedAction = 0;
    pressure = 0.0f;
    prisonerStress = 0.0f;
    silenceTimer = 0.0f;
    finished = false;
    verdictMercy = false;
    reactionLine.clear();
    reactionTimer = 0.0f;
}
