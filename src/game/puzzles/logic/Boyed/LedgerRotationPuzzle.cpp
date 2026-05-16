#include "game/puzzles/logic/Boyed/LedgerRotationPuzzle.h"

#include <algorithm>
#include <sstream>

#include <glm/gtc/matrix_transform.hpp>

#include "engine/renderer/TextRenderer.h"

LedgerRotationPuzzle::LedgerRotationPuzzle(std::string title, std::string clue, std::string solvedMessage)
    : title(std::move(title)), clue(std::move(clue)), solvedMessage(std::move(solvedMessage)) {
    UpdateTransforms();
}

int LedgerRotationPuzzle::NormalizeRotation(int rotation) {
    rotation %= 4;
    if (rotation < 0) {
        rotation += 4;
    }
    return rotation;
}

void LedgerRotationPuzzle::UpdateTransforms() {
    for (std::size_t i = 0; i < nodeTransforms.size(); ++i) {
        const float angle = glm::radians(static_cast<float>(currentRotation[i] * 90));
        nodeTransforms[i] = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 0.0f, 1.0f));
    }
}

bool LedgerRotationPuzzle::MatchesTarget() const {
    for (std::size_t i = 0; i < currentRotation.size(); ++i) {
        if (currentRotation[i] != targetRotation[i]) {
            return false;
        }
    }
    return true;
}

void LedgerRotationPuzzle::Start() {
    PlaySound("puzzle_open");
}

void LedgerRotationPuzzle::Update(float, const InputContext& input) {
    if (solved) {
        return;
    }

    for (int option = 0; option < 3; ++option) {
            if (input.IsActionPressed(static_cast<InputAction>(ToIndex(InputAction::PuzzleOption1) + option))) {
            selectedNode = option;
        }
    }

        if (input.IsActionPressed(InputAction::PuzzleDecrease)) {
            currentRotation[selectedNode] = NormalizeRotation(currentRotation[selectedNode] - 1);
        UpdateTransforms();
        PlaySound("puzzle_tick");
    }

        if (input.IsActionPressed(InputAction::PuzzleIncrease)) {
            currentRotation[selectedNode] = NormalizeRotation(currentRotation[selectedNode] + 1);
        UpdateTransforms();
        PlaySound("puzzle_tick");
    }

        if (input.IsActionPressed(InputAction::PuzzleReset)) {
            Reset();
        PlaySound("puzzle_fail");
    }

    if (MatchesTarget()) {
        solved = true;
        PlaySound("puzzle_solve");
    }
}

void LedgerRotationPuzzle::Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const {
    const glm::vec3 titleColor(1.0f, 0.2f, 0.2f);
    const glm::vec3 textColor(0.92f, 0.92f, 0.85f);
    const glm::vec3 accentColor(0.70f, 0.90f, 1.0f);
    const float baseY = static_cast<float>(screenHeight) * 0.56f;

    textRenderer.RenderText(title, 74.0f, static_cast<float>(screenHeight) - 92.0f, 0.66f, titleColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(clue, 74.0f, static_cast<float>(screenHeight) - 130.0f, 0.42f, accentColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("ROTATE THE LEDGER SEALS", 74.0f, static_cast<float>(screenHeight) - 168.0f, 0.38f, textColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("1-3 SELECT  Q/E ROTATE  R RESET", 74.0f, static_cast<float>(screenHeight) - 200.0f, 0.35f, textColor * alpha, screenWidth, screenHeight);

    for (int i = 0; i < 3; ++i) {
        std::ostringstream line;
        line << (i + 1) << ". NODE " << static_cast<char>('A' + i)
             << "  ROT " << currentRotation[i] * 90 << "°"
             << (i == selectedNode ? "  <" : "");
        const glm::vec3 color = (i == selectedNode) ? glm::vec3(1.0f, 0.6f, 0.45f) : glm::vec3(0.86f, 0.86f, 0.78f);
        textRenderer.RenderText(line.str(), 74.0f, baseY - (static_cast<float>(i) * 34.0f), 0.44f, color * alpha, screenWidth, screenHeight);
    }

    std::ostringstream target;
    target << "TARGET ALIGNMENT: " << targetRotation[0] * 90 << " / " << targetRotation[1] * 90 << " / " << targetRotation[2] * 90;
    textRenderer.RenderText(target.str(), 74.0f, baseY - 130.0f, 0.40f, accentColor * alpha, screenWidth, screenHeight);

    if (solved) {
        textRenderer.RenderText(solvedMessage, 74.0f, baseY - 170.0f, 0.45f, glm::vec3(1.0f, 0.35f, 0.35f) * alpha, screenWidth, screenHeight);
    }

    // Draw clear selectable markers for the three ledger seals below the text
    const float centerX = static_cast<float>(screenWidth) * 0.5f;
    const float markerY = baseY - 260.0f;
    const float spacing = 140.0f;
    for (int i = 0; i < 3; ++i) {
        const float markerX = centerX + (static_cast<float>(i) - 1.0f) * spacing;
        const bool isSelected = (i == selectedNode);
        const glm::vec3 bgColor = isSelected ? glm::vec3(0.18f, 0.12f, 0.08f) : glm::vec3(0.12f, 0.14f, 0.16f);
        const glm::vec3 fgColor = isSelected ? glm::vec3(1.0f, 0.85f, 0.7f) : glm::vec3(0.9f, 0.9f, 0.86f);

        // big filled square as a tactile background 'button'
        textRenderer.RenderText("■", markerX - 24.0f, markerY + 8.0f, 4.4f, bgColor * alpha, screenWidth, screenHeight);

        // letter label on top
        std::ostringstream label;
        label << static_cast<char>('A' + i);
        textRenderer.RenderText(label.str(), markerX - 12.0f, markerY - 12.0f, 2.2f, fgColor * alpha, screenWidth, screenHeight);

        // rotation indicator under the marker
        std::ostringstream rot;
        rot << currentRotation[i] * 90 << "°";
        textRenderer.RenderText(rot.str(), markerX - 16.0f, markerY - 90.0f, 0.7f, glm::vec3(0.95f, 0.95f, 0.9f) * alpha, screenWidth, screenHeight);

        if (isSelected) {
            textRenderer.RenderText("Q/E to rotate", markerX - 48.0f, markerY + 80.0f, 0.52f, glm::vec3(1.0f, 0.85f, 0.7f) * alpha, screenWidth, screenHeight);
        }
    }

    // explicit control hint centered
    textRenderer.RenderText("Press 1-3 to select a seal  |  Q/E rotate  |  R reset", centerX - 360.0f, baseY - 380.0f, 0.48f, glm::vec3(0.95f, 0.95f, 0.9f) * alpha, screenWidth, screenHeight);
}

std::string LedgerRotationPuzzle::SerializeState() const {
    std::ostringstream stream;
    stream << (solved ? 1 : 0) << ';' << selectedNode << ';';
    for (std::size_t i = 0; i < currentRotation.size(); ++i) {
        if (i > 0) {
            stream << ',';
        }
        stream << currentRotation[i];
    }
    return stream.str();
}

void LedgerRotationPuzzle::DeserializeState(const std::string& state) {
    std::stringstream stream(state);
    std::string token;

    std::getline(stream, token, ';');
    solved = token == "1";
    std::getline(stream, token, ';');
    selectedNode = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';');
    std::stringstream rotations(token);
    for (std::size_t i = 0; i < currentRotation.size(); ++i) {
        std::getline(rotations, token, ',');
        currentRotation[i] = token.empty() ? 0 : std::stoi(token);
    }
    UpdateTransforms();
}

void LedgerRotationPuzzle::Reset() {
    currentRotation = {0, 0, 0};
    selectedNode = 0;
    solved = false;
    UpdateTransforms();
}