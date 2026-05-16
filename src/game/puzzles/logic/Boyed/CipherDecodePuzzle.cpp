#include "game/puzzles/logic/Boyed/CipherDecodePuzzle.h"

#include <sstream>

#include "engine/renderer/TextRenderer.h"

CipherDecodePuzzle::CipherDecodePuzzle(std::string title, std::string clue, std::string encoded, std::array<std::string, 4> options, int correctIndex, std::string solvedMessage)
    : title(std::move(title)), clue(std::move(clue)), encoded(std::move(encoded)), options(std::move(options)), correctIndex(correctIndex), solvedMessage(std::move(solvedMessage)) {
}

void CipherDecodePuzzle::Start() {
    PlaySound("puzzle_open");
}

void CipherDecodePuzzle::Update(float, const InputContext& input) {
    if (solved) {
        return;
    }

    for (int option = 0; option < 4; ++option) {
        if (input.IsActionPressed(static_cast<InputAction>(ToIndex(InputAction::PuzzleOption1) + option))) {
            selectedIndex = option;
            if (selectedIndex == correctIndex) {
                solved = true;
                PlaySound("puzzle_solve");
            } else {
                PlaySound("puzzle_fail");
            }
        }
    }
}

void CipherDecodePuzzle::Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const {
    const glm::vec3 titleColor(1.0f, 0.2f, 0.2f);
    const glm::vec3 accentColor(0.70f, 0.90f, 1.0f);
    const glm::vec3 textColor(0.92f, 0.92f, 0.85f);

    textRenderer.RenderText(title, 74.0f, static_cast<float>(screenHeight) - 92.0f, 0.82f, titleColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(clue, 74.0f, static_cast<float>(screenHeight) - 130.0f, 0.54f, accentColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("DECODE THE TOWN LEDGER", 74.0f, static_cast<float>(screenHeight) - 168.0f, 0.50f, textColor * alpha, screenWidth, screenHeight);

    textRenderer.RenderText(encoded, 74.0f, static_cast<float>(screenHeight) * 0.58f, 0.56f, accentColor * alpha, screenWidth, screenHeight);

    for (int i = 0; i < 4; ++i) {
        std::ostringstream line;
        line << (i + 1) << ". " << options[i];
        glm::vec3 color = (i == selectedIndex) ? glm::vec3(1.0f, 0.35f, 0.35f) : textColor;
        textRenderer.RenderText(line.str(), 74.0f, static_cast<float>(screenHeight) * 0.49f - (static_cast<float>(i) * 24.0f), 0.50f, color * alpha, screenWidth, screenHeight);
    }

    if (solved) {
        textRenderer.RenderText(solvedMessage, 74.0f, static_cast<float>(screenHeight) * 0.31f, 0.56f, glm::vec3(1.0f, 0.35f, 0.35f) * alpha, screenWidth, screenHeight);
    }
}

std::string CipherDecodePuzzle::SerializeState() const {
    return solved ? "1" : "0";
}

void CipherDecodePuzzle::DeserializeState(const std::string& state) {
    solved = state == "1";
}

void CipherDecodePuzzle::Reset() {
    solved = false;
    selectedIndex = -1;
}