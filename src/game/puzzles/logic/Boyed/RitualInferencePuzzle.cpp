#include "game/puzzles/logic/Boyed/RitualInferencePuzzle.h"

#include <algorithm>
#include <sstream>

#include <GLFW/glfw3.h>

#include "engine/renderer/TextRenderer.h"

RitualInferencePuzzle::RitualInferencePuzzle(std::string title, std::string clue, std::string solvedMessage)
    : title(std::move(title)), clue(std::move(clue)), solvedMessage(std::move(solvedMessage)) {
}

void RitualInferencePuzzle::Start() {
    PlaySound("puzzle_open");
    cluesSatisfied = EvaluateClues();
}

int RitualInferencePuzzle::EvaluateClues() const {
    int score = 0;

    // Clue 1: North must hold ROOT.
    if (assignment[0] == 0) {
        ++score;
    }

    // Clue 2: East cannot hold THREAD.
    if (assignment[1] != 2) {
        ++score;
    }

    // Clue 3: WEST may not hold MIRROR.
    if (assignment[2] != 1) {
        ++score;
    }

    // Clue 4: Exactly one brazier can use MIRROR and it must be EAST.
    const int mirrorCount = (assignment[0] == 1 ? 1 : 0) + (assignment[1] == 1 ? 1 : 0) + (assignment[2] == 1 ? 1 : 0);
    if (mirrorCount == 1 && assignment[1] == 1) {
        ++score;
    }

    return score;
}

void RitualInferencePuzzle::CheckSolution() {
    cluesSatisfied = EvaluateClues();
    if (assignment == target) {
        solved = true;
        statusLine = solvedMessage;
        statusTimer = 2.5f;
        PlaySound("puzzle_solve");
        return;
    }

    std::ostringstream hint;
    hint << cluesSatisfied << "/4 clues satisfied. Refine the pattern.";
    statusLine = hint.str();
    statusTimer = 1.8f;
    PlaySound("puzzle_tick");
}

void RitualInferencePuzzle::Update(float dt, const InputContext& input) {
    statusTimer = std::max(0.0f, statusTimer - dt);

    if (input.IsActionPressed(InputAction::PuzzleReset)) {
        Reset();
        PlaySound("puzzle_open");
        return;
    }

    if (solved) {
        return;
    }

    for (int option = 0; option < 3; ++option) {
        if (input.IsActionPressed(static_cast<InputAction>(ToIndex(InputAction::PuzzleOption1) + option))) {
            selectedBrazier = option;
        }
    }

    if (input.IsActionPressed(InputAction::PuzzleDecrease)) {
        assignment[selectedBrazier] = (assignment[selectedBrazier] + 2) % 3;
        cluesSatisfied = EvaluateClues();
        PlaySound("puzzle_tick");
    }

    if (input.IsActionPressed(InputAction::PuzzleIncrease)) {
        assignment[selectedBrazier] = (assignment[selectedBrazier] + 1) % 3;
        cluesSatisfied = EvaluateClues();
        PlaySound("puzzle_tick");
    }

    if (input.IsActionPressed(InputAction::PuzzleConfirm)) {
        CheckSolution();
    }
}

void RitualInferencePuzzle::Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const {
    const float centerX = static_cast<float>(screenWidth) * 0.5f;
    const float baseY = static_cast<float>(screenHeight) * 0.28f;
    const glm::vec3 titleColor(0.8f, 0.95f, 1.0f);
    const glm::vec3 warnColor(1.0f, 0.82f, 0.42f);
    const glm::vec3 okColor(0.65f, 1.0f, 0.7f);
    const glm::vec3 accentColor(0.72f, 0.9f, 1.0f);

    const bool clue1Ok = (assignment[0] == 0);
    const bool clue2Ok = (assignment[1] != 2);
    const bool clue3Ok = (assignment[2] != 1);
    const int mirrorCount = (assignment[0] == 1 ? 1 : 0) + (assignment[1] == 1 ? 1 : 0) + (assignment[2] == 1 ? 1 : 0);
    const bool clue4Ok = (mirrorCount == 1 && assignment[1] == 1);

    const auto clueColor = [&](bool ok) {
        return ok ? glm::vec3(0.55f, 1.0f, 0.55f) : glm::vec3(1.0f, 0.55f, 0.55f);
    };

    // Left Panel status (matching Jade's design)
    const float leftPanelX = 72.0f;
    const float leftPanelY = static_cast<float>(screenHeight) - 340.0f;

    textRenderer.RenderText("◇ RITUAL LIVE STATUS", leftPanelX, leftPanelY, 0.54f, warnColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("──────────────────────", leftPanelX, leftPanelY - 14.0f, 0.50f, glm::vec3(0.5f) * alpha, screenWidth, screenHeight);

    textRenderer.RenderText(std::string(clue1Ok ? "[OK] " : "[X] ") + "1) NORTH must hold ROOT", leftPanelX, leftPanelY - 40.0f, 0.39f, clueColor(clue1Ok) * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(std::string(clue2Ok ? "[OK] " : "[X] ") + "2) EAST cannot hold THREAD", leftPanelX, leftPanelY - 64.0f, 0.39f, clueColor(clue2Ok) * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(std::string(clue3Ok ? "[OK] " : "[X] ") + "3) WEST may not hold MIRROR", leftPanelX, leftPanelY - 88.0f, 0.39f, clueColor(clue3Ok) * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(std::string(clue4Ok ? "[OK] " : "[X] ") + "4) MIRROR once, only EAST", leftPanelX, leftPanelY - 112.0f, 0.39f, clueColor(clue4Ok) * alpha, screenWidth, screenHeight);

    std::ostringstream score;
    score << "Consistency: " << cluesSatisfied << "/4 satisfied";
    textRenderer.RenderText(score.str(), leftPanelX, leftPanelY - 142.0f, 0.44f, accentColor * alpha, screenWidth, screenHeight);

    if (!statusLine.empty() && statusTimer > 0.0f) {
        const glm::vec3 statusColor = solved ? okColor : warnColor;
        textRenderer.RenderText(statusLine, leftPanelX, leftPanelY - 176.0f, 0.44f, statusColor * alpha, screenWidth, screenHeight);
    }

    // Centered interactive board
    std::ostringstream legend;
    legend << "SIGILS: ROOT (anchor) | MIRROR (reflect) | THREAD (bind)";
    float legendX = centerX - (static_cast<float>(legend.str().length()) * 4.4f);
    textRenderer.RenderText(legend.str(), legendX, baseY + 180.0f, 0.48f, glm::vec3(0.78f, 0.88f, 0.98f) * alpha, screenWidth, screenHeight);

    float spacing = 150.0f;
    float markerY = baseY + 60.0f;
    for (int i = 0; i < 3; ++i) {
        const float markerX = centerX + (static_cast<float>(i) - 1.0f) * spacing;
        const bool isSelected = (i == selectedBrazier);
        const glm::vec3 bgColor = isSelected ? glm::vec3(0.18f, 0.12f, 0.08f) : glm::vec3(0.12f, 0.14f, 0.16f);
        const glm::vec3 fgColor = isSelected ? glm::vec3(1.0f, 0.85f, 0.7f) : glm::vec3(0.9f, 0.9f, 0.86f);

        // big filled square
        textRenderer.RenderText("■", markerX - 24.0f, markerY + 8.0f, 4.4f, bgColor * alpha, screenWidth, screenHeight);

        // Label on top
        std::string label = braziers[i];
        float labelX = markerX - (static_cast<float>(label.length()) * 5.0f);
        textRenderer.RenderText(label, labelX, markerY - 12.0f, 0.65f, fgColor * alpha, screenWidth, screenHeight);

        // Sigil under
        std::string sigilLabel = sigils[assignment[i]];
        float sigilX = markerX - (static_cast<float>(sigilLabel.length()) * 5.0f);
        textRenderer.RenderText(sigilLabel, sigilX, markerY - 60.0f, 0.60f, glm::vec3(0.95f, 0.95f, 0.9f) * alpha, screenWidth, screenHeight);

        if (isSelected) {
            textRenderer.RenderText("Q/E to shift", markerX - 48.0f, markerY + 80.0f, 0.52f, glm::vec3(1.0f, 0.85f, 0.7f) * alpha, screenWidth, screenHeight);
        }
    }

    if (solved) {
        textRenderer.RenderText("★ RITUAL COMPLETED ★", centerX - 160.0f, baseY + 200.0f, 1.25f, okColor * alpha, screenWidth, screenHeight);
    }
}

std::string RitualInferencePuzzle::SerializeState() const {
    std::ostringstream stream;
    stream << (solved ? 1 : 0) << ';'
           << selectedBrazier << ';'
           << assignment[0] << ',' << assignment[1] << ',' << assignment[2];
    return stream.str();
}

void RitualInferencePuzzle::DeserializeState(const std::string& state) {
    std::stringstream stream(state);
    std::string token;

    std::getline(stream, token, ';');
    solved = token == "1";
    std::getline(stream, token, ';');
    selectedBrazier = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';');

    std::stringstream assignmentStream(token);
    for (int i = 0; i < 3; ++i) {
        std::getline(assignmentStream, token, ',');
        assignment[i] = token.empty() ? i : std::stoi(token);
        assignment[i] = std::clamp(assignment[i], 0, 2);
    }

    selectedBrazier = std::clamp(selectedBrazier, 0, 2);
    cluesSatisfied = EvaluateClues();
}

void RitualInferencePuzzle::Reset() {
    assignment = {2, 0, 1};
    selectedBrazier = 0;
    cluesSatisfied = EvaluateClues();
    solved = false;
    statusLine.clear();
    statusTimer = 0.0f;
}
