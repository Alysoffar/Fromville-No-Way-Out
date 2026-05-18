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
    const float centerX = static_cast<float>(screenWidth) * 0.5f;
    const float baseY = static_cast<float>(screenHeight) * 0.28f;
    const glm::vec3 titleColor(0.8f, 0.95f, 1.0f);
    const glm::vec3 warnColor(1.0f, 0.82f, 0.42f);
    const glm::vec3 okColor(0.65f, 1.0f, 0.7f);
    const glm::vec3 accentColor(0.70f, 0.90f, 1.0f);

    // Left Panel status (matching Jade's design)
    const float leftPanelX = 72.0f;
    const float leftPanelY = static_cast<float>(screenHeight) - 340.0f;

    textRenderer.RenderText("◇ DECIPHER STATUS", leftPanelX, leftPanelY, 0.54f, warnColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("──────────────────────", leftPanelX, leftPanelY - 14.0f, 0.50f, glm::vec3(0.5f) * alpha, screenWidth, screenHeight);
    
    for (int i = 0; i < 3; ++i) {
        std::ostringstream line;
        line << "Node " << static_cast<char>('A' + i) << " Rot: " << currentRotation[i] * 90 << "°";
        const glm::vec3 color = (i == selectedNode) ? okColor : glm::vec3(0.86f, 0.86f, 0.78f);
        textRenderer.RenderText(line.str(), leftPanelX, leftPanelY - 40.0f - (static_cast<float>(i) * 24.0f), 0.52f, color * alpha, screenWidth, screenHeight);
    }

    std::ostringstream target;
    target << "Target: " << targetRotation[0] * 90 << "° / " << targetRotation[1] * 90 << "° / " << targetRotation[2] * 90 << "°";
    textRenderer.RenderText(target.str(), leftPanelX, leftPanelY - 120.0f, 0.50f, accentColor * alpha, screenWidth, screenHeight);

    if (solved) {
        textRenderer.RenderText("Solved:", leftPanelX, leftPanelY - 150.0f, 0.50f, okColor * alpha, screenWidth, screenHeight);
        textRenderer.RenderText("Alignment verified.", leftPanelX, leftPanelY - 174.0f, 0.46f, okColor * alpha, screenWidth, screenHeight);
    }

    // Centered interactive board
    const float markerY = baseY + 60.0f;
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

    if (solved) {
        textRenderer.RenderText("★ LEDGER UNLOCKED ★", centerX - 150.0f, baseY + 200.0f, 1.25f, okColor * alpha, screenWidth, screenHeight);
    }
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