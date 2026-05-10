#include "game/puzzles/ConnectTheCluesPuzzle.h"

#include <algorithm>
#include <sstream>

#include "engine/renderer/TextRenderer.h"

ConnectTheCluesPuzzle::ConnectTheCluesPuzzle(std::string title, std::string clue, std::vector<std::string> clues, std::array<int, 4> answerOrder, std::string solvedMessage)
    : title(std::move(title)), clue(std::move(clue)), clues(std::move(clues)), answerOrder(answerOrder), solvedMessage(std::move(solvedMessage)) {
}

void ConnectTheCluesPuzzle::Start() {
    PlaySound("puzzle_open");
}

void ConnectTheCluesPuzzle::Update(float, const InputManager& input) {
    if (solved) {
        return;
    }

    for (int option = 0; option < 4; ++option) {
        if (input.IsKeyPressed(GLFW_KEY_1 + option)) {
            chosenOrder.push_back(option);
            if (chosenOrder.size() > answerOrder.size()) {
                chosenOrder.clear();
                PlaySound("puzzle_fail");
                return;
            }

            const std::size_t current = chosenOrder.size() - 1;
            if (answerOrder[current] != option) {
                chosenOrder.clear();
                PlaySound("puzzle_fail");
                return;
            }

            PlaySound("puzzle_tick");
            if (chosenOrder.size() == answerOrder.size()) {
                solved = true;
                PlaySound("puzzle_solve");
            }
        }
    }
}

void ConnectTheCluesPuzzle::Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const {
    const glm::vec3 titleColor(1.0f, 0.2f, 0.2f);
    const glm::vec3 accentColor(0.70f, 0.90f, 1.0f);
    const glm::vec3 textColor(0.92f, 0.92f, 0.85f);

    textRenderer.RenderText(title, 74.0f, static_cast<float>(screenHeight) - 92.0f, 0.66f, titleColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(clue, 74.0f, static_cast<float>(screenHeight) - 130.0f, 0.42f, accentColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("CONNECT THE CLUES ON THE BOARD", 74.0f, static_cast<float>(screenHeight) - 168.0f, 0.38f, textColor * alpha, screenWidth, screenHeight);

    float y = static_cast<float>(screenHeight) * 0.58f;
    for (std::size_t i = 0; i < clues.size(); ++i) {
        std::ostringstream line;
        line << (i + 1) << ". " << clues[i];
        glm::vec3 color = (std::find(chosenOrder.begin(), chosenOrder.end(), static_cast<int>(i)) != chosenOrder.end())
            ? glm::vec3(1.0f, 0.35f, 0.35f)
            : textColor;
        textRenderer.RenderText(line.str(), 74.0f, y - (static_cast<float>(i) * 24.0f), 0.38f, color * alpha, screenWidth, screenHeight);
    }

    if (!chosenOrder.empty()) {
        std::ostringstream progress;
        progress << "LINKED: " << chosenOrder.size() << "/" << answerOrder.size();
        textRenderer.RenderText(progress.str(), 74.0f, y - 124.0f, 0.40f, accentColor * alpha, screenWidth, screenHeight);
    }

    if (solved) {
        textRenderer.RenderText(solvedMessage, 74.0f, y - 170.0f, 0.45f, glm::vec3(1.0f, 0.35f, 0.35f) * alpha, screenWidth, screenHeight);
    }
}

std::string ConnectTheCluesPuzzle::SerializeState() const {
    return solved ? "1" : "0";
}

void ConnectTheCluesPuzzle::DeserializeState(const std::string& state) {
    solved = state == "1";
}

void ConnectTheCluesPuzzle::Reset() {
    solved = false;
    chosenOrder.clear();
}