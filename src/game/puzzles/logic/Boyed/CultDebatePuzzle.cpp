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
    const glm::vec3 titleColor(1.0f, 0.25f, 0.25f);
    const glm::vec3 textColor(0.92f, 0.92f, 0.86f);
    const glm::vec3 accentColor(0.75f, 0.92f, 1.0f);
    const float baseY = static_cast<float>(screenHeight) * 0.58f;

    textRenderer.RenderText(title, 74.0f, static_cast<float>(screenHeight) - 92.0f, 0.66f, titleColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(clue, 74.0f, static_cast<float>(screenHeight) - 130.0f, 0.42f, accentColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("DEBATE THE CULT LEADER (pick your words)", 74.0f, static_cast<float>(screenHeight) - 168.0f, 0.38f, textColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("1-3 SELECT RESPONSE  ENTER/SPACE COMMIT  R RESTART", 74.0f, static_cast<float>(screenHeight) - 200.0f, 0.35f, textColor * alpha, screenWidth, screenHeight);

    std::ostringstream score;
    score << "Strong responses: " << strongResponses << "/" << rounds.size();
    textRenderer.RenderText(score.str(), 74.0f, static_cast<float>(screenHeight) - 236.0f, 0.36f, glm::vec3(1.0f, 0.86f, 0.62f) * alpha, screenWidth, screenHeight);

    if (!solved && roundIndex < static_cast<int>(rounds.size())) {
        const DebateRound& round = rounds[roundIndex];
        std::ostringstream roundLabel;
        roundLabel << "ROUND " << (roundIndex + 1) << "/" << rounds.size();
        textRenderer.RenderText(roundLabel.str(), 74.0f, baseY + 54.0f, 0.44f, accentColor * alpha, screenWidth, screenHeight);
        textRenderer.RenderText(round.accusation, 74.0f, baseY + 18.0f, 0.43f, textColor * alpha, screenWidth, screenHeight);

        for (int i = 0; i < 3; ++i) {
            std::ostringstream response;
            response << (i + 1) << ". " << round.responses[i];
            const glm::vec3 color = (i == selectedOption) ? glm::vec3(1.0f, 0.64f, 0.5f) : glm::vec3(0.86f, 0.86f, 0.78f);
            textRenderer.RenderText(response.str(), 74.0f, baseY - 30.0f - static_cast<float>(i) * 34.0f, 0.41f, color * alpha, screenWidth, screenHeight);
        }
    }

    if (!statusLine.empty() && statusTimer > 0.0f) {
        const glm::vec3 statusColor = solved ? glm::vec3(0.42f, 1.0f, 0.45f)
                                             : (failed ? glm::vec3(1.0f, 0.45f, 0.45f) : glm::vec3(1.0f, 0.9f, 0.65f));
        textRenderer.RenderText(statusLine, 74.0f, baseY - 170.0f, 0.44f, statusColor * alpha, screenWidth, screenHeight);
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
