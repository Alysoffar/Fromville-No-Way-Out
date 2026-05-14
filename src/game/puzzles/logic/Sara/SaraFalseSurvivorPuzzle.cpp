#include "game/puzzles/logic/Sara/SaraFalseSurvivorPuzzle.h"

#include <sstream>

#include "engine/renderer/TextRenderer.h"

SaraFalseSurvivorPuzzle::SaraFalseSurvivorPuzzle(std::string title, std::string clue, std::string solvedMessage)
    : title(std::move(title)), clue(std::move(clue)), solvedMessage(std::move(solvedMessage)) {}

void SaraFalseSurvivorPuzzle::Start() {
    Reset();
    survivorLine = "The survivor's voice is small; hands clench the blanket.";
    observationLine = "You notice the way their eyes flick toward the cracked window when you mention the child.";
    lineTimer = 3.0f;
    PlaySound("puzzle_open");
}

void SaraFalseSurvivorPuzzle::AdvanceProbe(int probe) {
    probeCount++;
    switch (probe) {
        case 0:
            survivorLine = "They answer a question too quickly — rehearsed, like a practiced lie.";
            observationLine = "Their breathing sharpens; a forced smile trembles on the lips.";
            suspicion += 0.35f;
            PlaySound("voice_quick");
            break;
        case 1:
            survivorLine = "They avoid looking at the photograph; fingers rub a spot on the wrist repeatedly.";
            observationLine = "A tremor runs through the hands when you ask about the church.";
            suspicion += 0.25f;
            PlaySound("fidget");
            break;
        case 2:
            survivorLine = "A laugh slips out where there should have been tears; the throat clicks oddly.";
            observationLine = "Their eyes dart to the door every time you whisper 'guards'.";
            suspicion += 0.30f;
            PlaySound("nervous_laugh");
            break;
        default:
            survivorLine = "Their answers blur; you cannot tell what is true and what is performance.";
            observationLine = "The room feels smaller; breath becomes a thing you both share.";
            suspicion += 0.1f;
            break;
    }
    lineTimer = 3.2f;
    suspicion = std::min(1.0f, suspicion);
}

void SaraFalseSurvivorPuzzle::Update(float dt, const InputContext& input) {
    if (input.IsActionPressed(InputAction::PuzzleReset)) { Start(); return; }
    if (concluded) return;

    lineTimer = std::max(0.0f, lineTimer - dt);

    if (input.IsActionPressed(InputAction::PuzzleOption1)) selectedProbe = 0;
    if (input.IsActionPressed(InputAction::PuzzleOption2)) selectedProbe = 1;
    if (input.IsActionPressed(InputAction::PuzzleOption3)) selectedProbe = 2;
    if (input.IsActionPressed(InputAction::PuzzleOption4)) selectedProbe = 3;

    if (input.IsActionPressed(InputAction::PuzzleConfirm)) {
        // If player chooses to judge now, we interpret suspicion
        if (probeCount < 2) {
            // encourage more observation
            AdvanceProbe(selectedProbe);
            PlaySound("probe_move");
        } else {
            // final judgement: trust or accuse
            if (suspicion >= 0.6f) {
                concluded = true;
                solvedMessage = "You refuse to lead them to safety; later you hear the place didn't survive.";
                PlaySound("judgement_hard");
            } else {
                concluded = true;
                solvedMessage = "You trust them. The next morning, steps vanish in the street; you are left wondering.";
                PlaySound("judgement_soft");
            }
        }
    }
}

void SaraFalseSurvivorPuzzle::Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const {
    const float topY = static_cast<float>(screenHeight) * 0.18f;
    const glm::vec3 titleColor(0.95f, 0.9f, 0.85f);
    const glm::vec3 cueColor(0.85f, 0.8f, 0.78f);

    textRenderer.RenderText(GetTitle(), 72.0f, topY, 0.72f, titleColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(title, 72.0f, topY + 34.0f, 0.34f, cueColor * alpha, screenWidth, screenHeight);

    if (!survivorLine.empty())
        textRenderer.RenderText(survivorLine, 72.0f, topY + 86.0f, 0.36f, glm::vec3(0.9f) * alpha, screenWidth, screenHeight);
    if (!observationLine.empty())
        textRenderer.RenderText(observationLine, 72.0f, topY + 128.0f, 0.32f, glm::vec3(0.8f,0.78f,0.76f) * alpha, screenWidth, screenHeight);

    // Probes options
    textRenderer.RenderText("[1] Ask about the child  [2] Show photograph  [3] Ask about the church  [4] Request hands", 72.0f, static_cast<float>(screenHeight) - 72.0f, 0.34f, glm::vec3(0.9f) * alpha, screenWidth, screenHeight);

    textRenderer.RenderText("Suspicion: " + std::to_string(static_cast<int>(suspicion * 100.0f)) + "%", 72.0f, static_cast<float>(screenHeight) - 110.0f, 0.34f, glm::vec3(1.0f,0.6f,0.6f) * alpha, screenWidth, screenHeight);

    if (concluded) {
        textRenderer.RenderText(solvedMessage, 72.0f, static_cast<float>(screenHeight) * 0.78f, 0.42f, glm::vec3(0.9f,0.8f,0.75f) * alpha, screenWidth, screenHeight);
    }
}

std::string SaraFalseSurvivorPuzzle::SerializeState() const {
    std::ostringstream stream;
    stream << (concluded ? 1 : 0) << ';' << selectedProbe << ';' << probeCount << ';' << suspicion;
    return stream.str();
}

void SaraFalseSurvivorPuzzle::DeserializeState(const std::string& state) {
    std::stringstream ss(state);
    std::string token;
    std::getline(ss, token, ';'); concluded = (token == "1");
    std::getline(ss, token, ';'); selectedProbe = token.empty() ? 0 : std::stoi(token);
    std::getline(ss, token, ';'); probeCount = token.empty() ? 0 : std::stoi(token);
    std::getline(ss, token, ';'); suspicion = token.empty() ? 0.0f : std::stof(token);
}

void SaraFalseSurvivorPuzzle::Reset() {
    selectedProbe = 0;
    probeCount = 0;
    suspicion = 0.0f;
    concluded = false;
    survivorLine.clear();
    observationLine.clear();
    lineTimer = 0.0f;
}
