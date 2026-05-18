#include "game/puzzles/logic/Boyed/MaraTrustDialoguePuzzle.h"

#include <algorithm>
#include <sstream>

#include <GLFW/glfw3.h>

#include "engine/renderer/TextRenderer.h"

MaraTrustDialoguePuzzle::MaraTrustDialoguePuzzle(std::string title, std::string clue, std::string solvedMessage)
    : title(std::move(title)), clue(std::move(clue)), solvedMessage(std::move(solvedMessage)),
      rounds{{
          {
              "Mara: Why should I trust you with this?",
              {
                  "I can force the truth out another way.",
                  "I will protect your name and the people around you.",
                  "Just talk. I do not have time for fear."
              },
              1,
              "Mara: Then you are no different from them."
          },
          {
              "Mara: If I speak, they will come for me tonight.",
              {
                  "Tell me where they gather and I will move you before dark.",
                  "That is your problem, not mine.",
                  "Hide and hope they pick someone else."
              },
              0,
              "Mara: You would leave me to die."
          },
          {
              "Mara: Last chance. Are you hunting glory or the truth?",
              {
                  "Glory. People follow strength.",
                  "Truth. Start with who leads the ritual.",
                  "Whatever gets me promoted fastest."
              },
              1,
              "Mara: I was wrong about you."
          }
      }} {
}

void MaraTrustDialoguePuzzle::Start() {
    PlaySound("puzzle_open");
}

void MaraTrustDialoguePuzzle::CommitChoice() {
    if (solved || failed) {
        return;
    }

    const DialogueRound& round = rounds[roundIndex];
    if (selectedOption != round.trustResponse) {
        failed = true;
        statusLine = round.failReaction + " Trust is broken. Press R to restart the conversation.";
        statusTimer = 4.0f;
        PlaySound("puzzle_fail");
        return;
    }

    ++roundIndex;
    statusLine = "Mara softens and gives you another piece of the truth.";
    statusTimer = 1.6f;
    PlaySound("puzzle_tick");

    if (roundIndex >= static_cast<int>(rounds.size())) {
        solved = true;
        statusLine = solvedMessage;
        statusTimer = 2.8f;
        PlaySound("puzzle_solve");
    } else {
        selectedOption = 0;
    }
}

void MaraTrustDialoguePuzzle::Update(float dt, const InputContext& input) {
    statusTimer = std::max(0.0f, statusTimer - dt);

    if (input.IsActionPressed(InputAction::PuzzleReset)) {
        Reset();
        PlaySound("puzzle_open");
        return;
    }

    if (solved || failed) {
        return;
    }

    for (int option = 0; option < 3; ++option) {
        if (input.IsActionPressed(static_cast<InputAction>(ToIndex(InputAction::PuzzleOption1) + option))) {
            selectedOption = option;
        }
    }

    if (input.IsActionPressed(InputAction::PuzzleConfirm)) {
        CommitChoice();
    }
}

void MaraTrustDialoguePuzzle::Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const {
    const float centerX = static_cast<float>(screenWidth) * 0.5f;
    const float baseY = static_cast<float>(screenHeight) * 0.28f;
    const glm::vec3 titleColor(0.8f, 0.95f, 1.0f);
    const glm::vec3 warnColor(1.0f, 0.82f, 0.42f);
    const glm::vec3 okColor(0.65f, 1.0f, 0.7f);
    const glm::vec3 textColor(0.92f, 0.92f, 0.86f);

    // Left Panel status (matching Jade's design)
    const float leftPanelX = 72.0f;
    const float leftPanelY = static_cast<float>(screenHeight) - 340.0f;

    textRenderer.RenderText("◇ INTERROGATION STATUS", leftPanelX, leftPanelY, 0.54f, warnColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("──────────────────────", leftPanelX, leftPanelY - 14.0f, 0.50f, glm::vec3(0.5f) * alpha, screenWidth, screenHeight);
    
    std::ostringstream progressStr;
    progressStr << "Round: " << std::min(roundIndex + 1, static_cast<int>(rounds.size())) << " / " << rounds.size();
    textRenderer.RenderText(progressStr.str(), leftPanelX, leftPanelY - 40.0f, 0.52f, titleColor * alpha, screenWidth, screenHeight);

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

    // Centered interactive board (matching Jade's design)
    if (!solved && roundIndex < static_cast<int>(rounds.size())) {
        const DialogueRound& round = rounds[roundIndex];
        
        std::ostringstream roundLabel;
        roundLabel << "✦ MARA'S INQUIRY ✦";
        float labelX = centerX - (static_cast<float>(roundLabel.str().length()) * 5.0f);
        textRenderer.RenderText(roundLabel.str(), labelX, baseY + 180.0f, 0.65f, warnColor * alpha, screenWidth, screenHeight);

        // Render Mara's accusation / line centered
        float accusationX = centerX - (static_cast<float>(round.maraLine.length()) * 4.2f);
        textRenderer.RenderText(round.maraLine, accusationX, baseY + 130.0f, 0.55f, textColor * alpha, screenWidth, screenHeight);

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
        textRenderer.RenderText("★ TRUST EARNED ★", centerX - 120.0f, baseY + 60.0f, 1.25f, okColor * alpha, screenWidth, screenHeight);
    }
}

std::string MaraTrustDialoguePuzzle::SerializeState() const {
    std::ostringstream stream;
    stream << (solved ? 1 : 0) << ';'
           << (failed ? 1 : 0) << ';'
           << roundIndex << ';'
           << selectedOption;
    return stream.str();
}

void MaraTrustDialoguePuzzle::DeserializeState(const std::string& state) {
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

    roundIndex = std::clamp(roundIndex, 0, static_cast<int>(rounds.size()));
    selectedOption = std::clamp(selectedOption, 0, 2);
}

void MaraTrustDialoguePuzzle::Reset() {
    roundIndex = 0;
    selectedOption = 0;
    solved = false;
    failed = false;
    statusLine.clear();
    statusTimer = 0.0f;
}
