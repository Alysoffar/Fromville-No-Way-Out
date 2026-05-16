#include "game/puzzles/logic/Boyed/SymbolMatchPuzzle.h"

#include <sstream>

#include "engine/renderer/TextRenderer.h"

SymbolMatchPuzzle::SymbolMatchPuzzle(std::string title, std::string clue, std::array<std::string, 4> symbols, int correctIndex, std::string solvedMessage)
    : title(std::move(title)), clue(std::move(clue)), symbols(std::move(symbols)), correctIndex(correctIndex), solvedMessage(std::move(solvedMessage)) {
}

void SymbolMatchPuzzle::Start() {
    PlaySound("puzzle_open");
}

void SymbolMatchPuzzle::Update(float, const InputContext& input) {
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

void SymbolMatchPuzzle::Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const {
    const glm::vec3 titleColor(1.0f, 0.2f, 0.2f);
    const glm::vec3 accentColor(0.70f, 0.90f, 1.0f);
    const glm::vec3 textColor(0.92f, 0.92f, 0.85f);

    textRenderer.RenderText(title, 74.0f, static_cast<float>(screenHeight) - 92.0f, 0.66f, titleColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(clue, 74.0f, static_cast<float>(screenHeight) - 130.0f, 0.42f, accentColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("MATCH THE SIGIL TO THE RIGHT NAME", 74.0f, static_cast<float>(screenHeight) - 168.0f, 0.38f, textColor * alpha, screenWidth, screenHeight);

    float y = static_cast<float>(screenHeight) * 0.58f;
    for (int i = 0; i < 4; ++i) {
        std::ostringstream line;
        line << (i + 1) << ". " << symbols[i];
        glm::vec3 color = (i == selectedIndex) ? glm::vec3(1.0f, 0.35f, 0.35f) : textColor;
        textRenderer.RenderText(line.str(), 74.0f, y - (static_cast<float>(i) * 26.0f), 0.44f, color * alpha, screenWidth, screenHeight);
    }

    if (solved) {
        textRenderer.RenderText(solvedMessage, 74.0f, y - 130.0f, 0.45f, glm::vec3(1.0f, 0.35f, 0.35f) * alpha, screenWidth, screenHeight);
    }
}

std::string SymbolMatchPuzzle::SerializeState() const {
    return solved ? "1" : "0";
}

void SymbolMatchPuzzle::DeserializeState(const std::string& state) {
    solved = state == "1";
}

void SymbolMatchPuzzle::Reset() {
    solved = false;
    selectedIndex = -1;
}