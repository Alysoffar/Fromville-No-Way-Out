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
    const glm::vec3 titleColor(1.0f, 0.2f, 0.2f);
    const glm::vec3 textColor(0.92f, 0.92f, 0.85f);
    const glm::vec3 accentColor(0.72f, 0.9f, 1.0f);
    const float baseY = static_cast<float>(screenHeight) * 0.58f;

    const bool clue1Ok = (assignment[0] == 0);
    const bool clue2Ok = (assignment[1] != 2);
    const bool clue3Ok = (assignment[2] != 1);
    const int mirrorCount = (assignment[0] == 1 ? 1 : 0) + (assignment[1] == 1 ? 1 : 0) + (assignment[2] == 1 ? 1 : 0);
    const bool clue4Ok = (mirrorCount == 1 && assignment[1] == 1);

    const auto clueColor = [&](bool ok) {
        return ok ? glm::vec3(0.55f, 1.0f, 0.55f) : glm::vec3(1.0f, 0.55f, 0.55f);
    };

    textRenderer.RenderText(title, 74.0f, static_cast<float>(screenHeight) - 92.0f, 0.66f, titleColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(clue, 74.0f, static_cast<float>(screenHeight) - 130.0f, 0.42f, accentColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("INFER THE COUNTER-RITUAL (logic, not speed)", 74.0f, static_cast<float>(screenHeight) - 168.0f, 0.38f, textColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("HOW TO SOLVE: Pick a brazier (1-3), rotate its sigil (Q/E), then press ENTER to verify.", 74.0f, static_cast<float>(screenHeight) - 200.0f, 0.35f, textColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("Goal: make ALL 4 clues green at the same time. Wrong setup will not progress.", 74.0f, static_cast<float>(screenHeight) - 230.0f, 0.35f, glm::vec3(1.0f, 0.86f, 0.62f) * alpha, screenWidth, screenHeight);

    textRenderer.RenderText("CLUES (live status):", 74.0f, baseY + 56.0f, 0.43f, glm::vec3(1.0f, 0.86f, 0.62f) * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(std::string(clue1Ok ? "[OK] " : "[X] ") + "1) NORTH must hold ROOT", 74.0f, baseY + 26.0f, 0.39f, clueColor(clue1Ok) * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(std::string(clue2Ok ? "[OK] " : "[X] ") + "2) EAST cannot hold THREAD", 74.0f, baseY - 2.0f, 0.39f, clueColor(clue2Ok) * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(std::string(clue3Ok ? "[OK] " : "[X] ") + "3) WEST may not hold MIRROR", 74.0f, baseY - 30.0f, 0.39f, clueColor(clue3Ok) * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(std::string(clue4Ok ? "[OK] " : "[X] ") + "4) MIRROR appears once and only on EAST", 74.0f, baseY - 58.0f, 0.39f, clueColor(clue4Ok) * alpha, screenWidth, screenHeight);

    textRenderer.RenderText("SIGIL LEGEND: ROOT = anchor  |  MIRROR = reflect  |  THREAD = bind", 74.0f, baseY - 84.0f, 0.34f, glm::vec3(0.78f, 0.88f, 0.98f) * alpha, screenWidth, screenHeight);

    for (int i = 0; i < 3; ++i) {
        std::ostringstream line;
        line << (i + 1) << ". " << braziers[i] << " BRAZIER -> " << sigils[assignment[i]];
        const glm::vec3 color = (i == selectedBrazier) ? glm::vec3(1.0f, 0.64f, 0.5f) : glm::vec3(0.86f, 0.86f, 0.78f);
        textRenderer.RenderText(line.str(), 74.0f, baseY - 112.0f - static_cast<float>(i) * 34.0f, 0.43f, color * alpha, screenWidth, screenHeight);
    }

    {
        std::ostringstream selected;
        selected << "Selected brazier: " << braziers[selectedBrazier] << " (press " << (selectedBrazier + 1) << ")";
        textRenderer.RenderText(selected.str(), 74.0f, baseY - 220.0f, 0.36f, glm::vec3(1.0f, 0.72f, 0.58f) * alpha, screenWidth, screenHeight);
    }

    std::ostringstream score;
    score << "Current consistency: " << cluesSatisfied << "/4";
    textRenderer.RenderText(score.str(), 74.0f, baseY - 248.0f, 0.40f, accentColor * alpha, screenWidth, screenHeight);

    if (!statusLine.empty() && statusTimer > 0.0f) {
        const glm::vec3 statusColor = solved ? glm::vec3(0.42f, 1.0f, 0.45f) : glm::vec3(1.0f, 0.9f, 0.65f);
        textRenderer.RenderText(statusLine, 74.0f, baseY - 284.0f, 0.43f, statusColor * alpha, screenWidth, screenHeight);
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
