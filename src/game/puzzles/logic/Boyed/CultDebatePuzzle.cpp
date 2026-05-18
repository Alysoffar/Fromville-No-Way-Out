#include "game/puzzles/logic/Boyed/CultDebatePuzzle.h"

#include <algorithm>
#include <sstream>

#include <GLFW/glfw3.h>

#include "engine/renderer/TextRenderer.h"

CultDebatePuzzle::CultDebatePuzzle(std::string title, std::string clue, std::string solvedMessage)
    : title(std::move(title)), clue(std::move(clue)), solvedMessage(std::move(solvedMessage)),
      rounds{{
          {"Leader: You have no proof. This town needs us.",
                     {"You are scary, but I can walk away.",
                        "Your own ledger ties every disappearance to your meetings.",
                        "Maybe we can strike a deal and forget this."},
                     1},
          {"Leader: Sacrifice keeps the monsters away. You would doom everyone.",
                     {"If one victim saves many, maybe you are right.",
                        "Then pick someone from your own circle first.",
                        "Fear is your weapon. The talismans protect people without murder."},
                     2},
          {"Leader: The townspeople follow me. They will never believe you.",
           {"They will when they hear your confession and see the ritual marks.",
            "I only need to scare them more than you do.",
            "Belief does not matter if I silence you here."},
           0}
      }} {
}

void CultDebatePuzzle::Start() {
    PlaySound("puzzle_open");
}

void CultDebatePuzzle::SubmitChoice() {
    if (failed || solved) {
        return;
    }

    if (selectedOption != rounds[roundIndex].winningResponse) {
        failed = true;
        statusLine = "He seizes your weak answer and turns the crowd against you. Press R to restart.";
        statusTimer = 4.0f;
        PlaySound("puzzle_fail");
        return;
    }

    ++strongResponses;
    statusLine = "You corner his argument.";
    statusTimer = 1.5f;
    ++roundIndex;
    PlaySound("puzzle_tick");

    if (roundIndex >= static_cast<int>(rounds.size())) {
        solved = true;
        statusLine = solvedMessage;
        statusTimer = 2.5f;
        PlaySound("puzzle_solve");
    } else {
        selectedOption = 0;
    }
}

void CultDebatePuzzle::Update(float dt, const InputContext& input) {
    statusTimer = std::max(0.0f, statusTimer - dt);

    if (input.IsActionPressed(InputAction::PuzzleReset)) {
        Reset();
        PlaySound("puzzle_open");
        return;
    }

    if (failed || solved) {
        return;
    }

    for (int option = 0; option < 3; ++option) {
            if (input.IsActionPressed(static_cast<InputAction>(ToIndex(InputAction::PuzzleOption1) + option))) {
                selectedOption = option;
        }
    }

        if (input.IsActionPressed(InputAction::PuzzleConfirm)) {
        SubmitChoice();
    }
}

void CultDebatePuzzle::Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const {
    const float centerX = static_cast<float>(screenWidth) * 0.5f;
    const float baseY = static_cast<float>(screenHeight) * 0.28f;
    const glm::vec3 titleColor(0.8f, 0.95f, 1.0f);
    const glm::vec3 warnColor(1.0f, 0.82f, 0.42f);
    const glm::vec3 okColor(0.65f, 1.0f, 0.7f);
    const glm::vec3 textColor(0.92f, 0.92f, 0.86f);
    const glm::vec3 accentColor(0.75f, 0.92f, 1.0f);

    // Left Panel status (matching Jade's design)
    const float leftPanelX = 72.0f;
    const float leftPanelY = static_cast<float>(screenHeight) - 340.0f;

    textRenderer.RenderText("◇ DEBATE STATUS", leftPanelX, leftPanelY, 0.54f, warnColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("──────────────────────", leftPanelX, leftPanelY - 14.0f, 0.50f, glm::vec3(0.5f) * alpha, screenWidth, screenHeight);
    
    std::ostringstream score;
    score << "Strong Response: " << strongResponses << " / " << rounds.size();
    textRenderer.RenderText(score.str(), leftPanelX, leftPanelY - 40.0f, 0.52f, titleColor * alpha, screenWidth, screenHeight);

    if (!statusLine.empty() && statusTimer > 0.0f) {
        textRenderer.RenderText("ANALYSIS:", leftPanelX, leftPanelY - 80.0f, 0.50f, okColor * alpha, screenWidth, screenHeight);
        const glm::vec3 statusColor = solved ? okColor : (failed ? glm::vec3(1.0f, 0.45f, 0.45f) : warnColor);
        if (statusLine.length() > 34) {
            std::string line1 = statusLine.substr(0, 32) + "...";
            std::string line2 = "..." + statusLine.substr(32);
            textRenderer.RenderText(line1, leftPanelX, leftPanelY - 104.0f, 0.46f, statusColor * alpha, screenWidth, screenHeight);
            textRenderer.RenderText(line2, leftPanelX, leftPanelY - 126.0f, 0.46f, statusColor * alpha, screenWidth, screenHeight);
        } else {
            textRenderer.RenderText(statusLine, leftPanelX, leftPanelY - 104.0f, 0.46f, statusColor * alpha, screenWidth, screenHeight);
        }
    }

    // Centered interactive board
    if (!solved && roundIndex < static_cast<int>(rounds.size())) {
        const DebateRound& round = rounds[roundIndex];
        
        std::ostringstream roundLabel;
        roundLabel << "✦ DEBATE ROUND " << (roundIndex + 1) << " / " << rounds.size() << " ✦";
        float labelX = centerX - (static_cast<float>(roundLabel.str().length()) * 5.0f);
        textRenderer.RenderText(roundLabel.str(), labelX, baseY + 180.0f, 0.65f, warnColor * alpha, screenWidth, screenHeight);

        // Render accusation centered
        float accusationX = centerX - (static_cast<float>(round.accusation.length()) * 4.2f);
        textRenderer.RenderText(round.accusation, accusationX, baseY + 130.0f, 0.55f, textColor * alpha, screenWidth, screenHeight);

        // Options centered
        float optionsY = baseY + 60.0f;
        for (int i = 0; i < 3; ++i) {
            std::ostringstream response;
            response << (i + 1) << ". " << round.responses[i];
            const glm::vec3 color = (selectedOption == i) ? okColor : glm::vec3(0.95f, 0.9f, 0.8f);
            float optX = centerX - 240.0f;
            textRenderer.RenderText(response.str(), optX, optionsY - static_cast<float>(i) * 32.0f, 0.52f, color * alpha, screenWidth, screenHeight);
        }
    } else if (solved) {
        textRenderer.RenderText("★ DEBATE WON ★", centerX - 110.0f, baseY + 60.0f, 1.25f, okColor * alpha, screenWidth, screenHeight);
    }
}

std::string CultDebatePuzzle::SerializeState() const {
    std::ostringstream stream;
    stream << (solved ? 1 : 0) << ';'
           << (failed ? 1 : 0) << ';'
           << roundIndex << ';'
           << selectedOption << ';'
           << strongResponses;
    return stream.str();
}

void CultDebatePuzzle::DeserializeState(const std::string& state) {
    std::stringstream stream(state);
    std::string token;

    std::getline(stream, token, ';');
    solved = token == "1";
    std::getline(stream, token, ';');
    failed = token == "1";
    std::getline(stream, token, ';');
    roundIndex = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';');
    selectedOption = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';');
    strongResponses = token.empty() ? 0 : std::stoi(token);

    roundIndex = std::clamp(roundIndex, 0, static_cast<int>(rounds.size()));
    selectedOption = std::clamp(selectedOption, 0, 2);
    strongResponses = std::clamp(strongResponses, 0, static_cast<int>(rounds.size()));
}

void CultDebatePuzzle::Reset() {
    roundIndex = 0;
    selectedOption = 0;
    strongResponses = 0;
    solved = false;
    failed = false;
    statusLine.clear();
    statusTimer = 0.0f;
}
