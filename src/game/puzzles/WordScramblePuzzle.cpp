#include "game/puzzles/WordScramblePuzzle.h"

#include <algorithm>
#include <cmath>
#include <sstream>

#include <glm/glm.hpp>

#include "engine/renderer/TextRenderer.h"

namespace {
std::string ToUpper(std::string value) {
    for (char& ch : value) {
        if (ch >= 'a' && ch <= 'z') {
            ch = static_cast<char>(ch - 'a' + 'A');
        }
    }
    return value;
}
}

WordScramblePuzzle::WordScramblePuzzle(std::string title, std::string clue, std::string answer, std::array<std::string, 4> options, std::string solvedMessage)
    : title(std::move(title)), clue(std::move(clue)), answer(std::move(answer)), options(std::move(options)), solvedMessage(std::move(solvedMessage)) {
}

void WordScramblePuzzle::Start() {
    PlaySound("puzzle_open");
}

void WordScramblePuzzle::Update(float dt, const InputManager& input) {
    if (solved) {
        solveTimer = std::max(0.0f, solveTimer - dt);
        return;
    }

    hoveredOption = -1;
    for (int option = 0; option < 4; ++option) {
        if (input.IsKeyPressed(GLFW_KEY_1 + option)) {
            hoveredOption = option;
            if (ToUpper(options[option]) == ToUpper(answer)) {
                solved = true;
                solveTimer = 1.5f;
                PlaySound("puzzle_solve");
            } else {
                PlaySound("puzzle_fail");
            }
        }
    }
}

void WordScramblePuzzle::Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const {
    const float baseY = static_cast<float>(screenHeight) * 0.58f;
    const glm::vec3 titleColor(1.0f, 0.2f, 0.2f);
    const glm::vec3 textColor(0.92f, 0.92f, 0.85f);
    const glm::vec3 clueColor(0.62f, 0.92f, 1.0f);
    const glm::vec3 accentColor(1.0f, 0.35f, 0.35f);

    textRenderer.RenderText(title, 74.0f, static_cast<float>(screenHeight) - 92.0f, 0.66f, titleColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(clue, 74.0f, static_cast<float>(screenHeight) - 130.0f, 0.42f, clueColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("UNSCRAMBLE THE FINAL MESSAGE", 74.0f, static_cast<float>(screenHeight) - 168.0f, 0.38f, textColor * alpha, screenWidth, screenHeight);

    std::ostringstream prompt;
    prompt << "1. " << options[0] << "    2. " << options[1] << "    3. " << options[2] << "    4. " << options[3];
    textRenderer.RenderText(prompt.str(), 74.0f, baseY, 0.44f, textColor * alpha, screenWidth, screenHeight);

    if (hoveredOption >= 0) {
        textRenderer.RenderText("YOUR CHOICE HAS BEEN RECORDED", 74.0f, baseY - 44.0f, 0.35f, accentColor * alpha, screenWidth, screenHeight);
    }

    if (solved) {
        textRenderer.RenderText(solvedMessage, 74.0f, baseY - 112.0f, 0.45f, accentColor * std::min(alpha + 0.25f, 1.0f), screenWidth, screenHeight);
    }
}

std::string WordScramblePuzzle::SerializeState() const {
    return solved ? "1" : "0";
}

void WordScramblePuzzle::DeserializeState(const std::string& state) {
    solved = state == "1";
}

void WordScramblePuzzle::Reset() {
    hoveredOption = -1;
    solved = false;
    solveTimer = 0.0f;
}