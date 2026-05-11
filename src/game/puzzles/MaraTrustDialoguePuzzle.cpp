#include "game/puzzles/MaraTrustDialoguePuzzle.h"

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

void MaraTrustDialoguePuzzle::Update(float dt, const InputManager& input) {
    statusTimer = std::max(0.0f, statusTimer - dt);

    if (input.IsKeyPressed(GLFW_KEY_R)) {
        Reset();
        PlaySound("puzzle_open");
        return;
    }

    if (solved || failed) {
        return;
    }

    for (int option = 0; option < 3; ++option) {
        if (input.IsKeyPressed(GLFW_KEY_1 + option)) {
            selectedOption = option;
        }
    }

    if (input.IsKeyPressed(GLFW_KEY_ENTER) || input.IsKeyPressed(GLFW_KEY_SPACE)) {
        CommitChoice();
    }
}

void MaraTrustDialoguePuzzle::Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const {
    const glm::vec3 titleColor(1.0f, 0.24f, 0.24f);
    const glm::vec3 textColor(0.92f, 0.92f, 0.86f);
    const glm::vec3 accentColor(0.75f, 0.92f, 1.0f);
    const float baseY = static_cast<float>(screenHeight) * 0.58f;

    textRenderer.RenderText(title, 74.0f, static_cast<float>(screenHeight) - 92.0f, 0.66f, titleColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(clue, 74.0f, static_cast<float>(screenHeight) - 130.0f, 0.42f, accentColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("EARN MARA'S TRUST (choose carefully)", 74.0f, static_cast<float>(screenHeight) - 168.0f, 0.38f, textColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("1-3 SELECT RESPONSE  ENTER/SPACE COMMIT  R RESTART", 74.0f, static_cast<float>(screenHeight) - 200.0f, 0.35f, textColor * alpha, screenWidth, screenHeight);

    std::ostringstream roundLabel;
    roundLabel << "DIALOGUE ROUND " << std::min(roundIndex + 1, static_cast<int>(rounds.size())) << "/" << rounds.size();
    textRenderer.RenderText(roundLabel.str(), 74.0f, static_cast<float>(screenHeight) - 236.0f, 0.36f, glm::vec3(1.0f, 0.86f, 0.62f) * alpha, screenWidth, screenHeight);

    if (!solved && roundIndex < static_cast<int>(rounds.size())) {
        const DialogueRound& round = rounds[roundIndex];
        textRenderer.RenderText(round.maraLine, 74.0f, baseY + 18.0f, 0.43f, textColor * alpha, screenWidth, screenHeight);

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
        textRenderer.RenderText(statusLine, 74.0f, baseY - 170.0f, 0.43f, statusColor * alpha, screenWidth, screenHeight);
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
