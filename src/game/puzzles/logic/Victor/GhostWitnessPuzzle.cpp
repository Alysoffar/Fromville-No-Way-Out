#include "game/puzzles/logic/Victor/GhostWitnessPuzzle.h"

#include <algorithm>
#include <sstream>

#include <glm/glm.hpp>

#include "engine/renderer/TextRenderer.h"

GhostWitnessPuzzle::GhostWitnessPuzzle(std::string title, std::string clue, std::string solvedMessage)
    : title(std::move(title)), clue(std::move(clue)), solvedMessage(std::move(solvedMessage)) {
}

void GhostWitnessPuzzle::Start() {
    Reset();
    statusLine = "The witnesses flicker between versions of the same story.";
    statusTimer = 2.6f;
    PlaySound("memory_fragment");
}

void GhostWitnessPuzzle::ToggleLineVariant() {
    lineVariant = 1 - lineVariant;
    ghostPulse = 1.0f;
    MemoryReplaySystem::Instance().TriggerGhostReplica(witnesses[static_cast<std::size_t>(selectedWitness)].name,
                                                       witnesses[static_cast<std::size_t>(selectedWitness)].lines[static_cast<std::size_t>(lineVariant)],
                                                       glm::vec3(0.0f));
}

void GhostWitnessPuzzle::AccuseWitness(int index) {
    if (index < 0 || index >= static_cast<int>(witnesses.size())) {
        return;
    }

    selectedWitness = index;
    const Witness& witness = witnesses[static_cast<std::size_t>(index)];
    if (witness.contradictionScore >= 3) {
        successfulInterrogations++;
        instability = std::max(0.0f, instability - 0.12f);
        statusLine = witness.name + " cannot keep the two timelines aligned.";
        statusTimer = 2.0f;
        PlaySound("puzzle_tick");
        if (successfulInterrogations >= 3) {
            solved = true;
            statusLine = "The ghosts merge their lies into one confession.";
            solvedMessage = "The ghost witness can no longer hide the betrayal.";
            PlaySound("puzzle_complete");
        } else {
            ToggleLineVariant();
        }
    } else {
        falseAccusations++;
        instability = std::min(1.0f, instability + 0.20f);
        statusLine = "Wrong witness. The room fractures a little more.";
        statusTimer = 2.0f;
        PlaySound("puzzle_fail");
        MemoryReplaySystem::Instance().IncraseTrauma(0.12f);
    }
}

void GhostWitnessPuzzle::Update(float dt, const InputContext& input) {
    ghostPulse = std::max(0.0f, ghostPulse - dt);
    statusTimer = std::max(0.0f, statusTimer - dt);
    instability = std::max(0.0f, instability - dt * 0.03f);

    if (input.IsActionPressed(InputAction::PuzzleReset)) {
        Start();
        return;
    }

    if (solved) {
        return;
    }

    if (input.IsActionPressed(InputAction::PuzzleOption1)) selectedWitness = 0;
    if (input.IsActionPressed(InputAction::PuzzleOption2)) selectedWitness = 1;
    if (input.IsActionPressed(InputAction::PuzzleOption3)) selectedWitness = 2;
    if (input.IsActionPressed(InputAction::PuzzleOption4)) selectedWitness = 3;

    if (input.IsActionPressed(InputAction::PuzzleConfirm)) {
        AccuseWitness(selectedWitness);
    }

    if (input.IsActionPressed(InputAction::PuzzleCancel)) {
        instability = std::min(1.0f, instability + 0.08f);
        statusLine = "The ghosts hear your hesitation and begin to overlap.";
        statusTimer = 1.6f;
        PlaySound("puzzle_tick");
    }

    if (input.IsActionPressed(InputAction::PuzzleIncrease)) {
        ToggleLineVariant();
        statusLine = "The witness shifts to another version of the same memory.";
        statusTimer = 1.6f;
        PlaySound("puzzle_tick");
    }
}

void GhostWitnessPuzzle::Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const {
    const float topY = static_cast<float>(screenHeight) * 0.18f;
    const glm::vec3 titleColor(0.76f, 0.86f, 1.0f);
    const glm::vec3 warnColor(1.0f, 0.8f, 0.48f);
    const glm::vec3 calmColor(0.72f, 0.95f, 0.72f);

    textRenderer.RenderText(GetTitle(), 72.0f, topY, 0.72f, titleColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(title, 72.0f, topY + 34.0f, 0.34f, warnColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(clue, 72.0f, topY + 62.0f, 0.32f, calmColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("Interrogate the witness whose memory cannot keep the same shape.", 72.0f, topY + 96.0f, 0.32f, warnColor * alpha, screenWidth, screenHeight);

    const Witness& witness = witnesses[static_cast<std::size_t>(selectedWitness)];
    textRenderer.RenderText("Witness: " + witness.name, 72.0f, topY + 140.0f, 0.40f, titleColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("Line A: " + witness.lines[static_cast<std::size_t>(lineVariant)], 72.0f, topY + 176.0f, 0.32f, calmColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("Line B: " + witness.lines[static_cast<std::size_t>(1 - lineVariant)], 72.0f, topY + 210.0f, 0.32f, warnColor * alpha, screenWidth, screenHeight);

    for (int i = 0; i < 4; ++i) {
        const glm::vec3 color = (selectedWitness == i) ? glm::vec3(1.0f, 0.95f, 0.5f) : calmColor;
        textRenderer.RenderText(std::to_string(i + 1) + ". " + witnesses[static_cast<std::size_t>(i)].name,
                                72.0f, topY + 252.0f + static_cast<float>(i) * 28.0f, 0.31f, color * alpha, screenWidth, screenHeight);
    }

    textRenderer.RenderText("Successful interrogations: " + std::to_string(successfulInterrogations) + "/3", 72.0f, static_cast<float>(screenHeight) - 160.0f, 0.36f, titleColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("Instability: " + std::string(static_cast<int>(instability * 20.0f), '!'), 72.0f, static_cast<float>(screenHeight) - 128.0f, 0.32f, warnColor * alpha, screenWidth, screenHeight);

    if (!statusLine.empty() && statusTimer > 0.0f) {
        textRenderer.RenderText(statusLine, 72.0f, static_cast<float>(screenHeight) - 92.0f, 0.34f, (solved ? calmColor : warnColor) * alpha, screenWidth, screenHeight);
    }

    if (solved) {
        textRenderer.RenderText("THE GHOST WITNESS CAN NO LONGER HIDE THE LIE", 72.0f, static_cast<float>(screenHeight) * 0.82f, 0.42f, calmColor * alpha, screenWidth, screenHeight);
    }

    textRenderer.RenderText("[1-4] choose witness  [ENTER] accuse  [E] change testimony  [ESC] steady  [R] restart",
                           72.0f, static_cast<float>(screenHeight) - 42.0f, 0.29f, titleColor * alpha, screenWidth, screenHeight);
}

std::string GhostWitnessPuzzle::SerializeState() const {
    std::ostringstream stream;
    stream << (solved ? 1 : 0) << ';' << selectedWitness << ';' << successfulInterrogations << ';' << falseAccusations << ';' << lineVariant << ';' << instability;
    return stream.str();
}

void GhostWitnessPuzzle::DeserializeState(const std::string& state) {
    std::stringstream stream(state);
    std::string token;

    std::getline(stream, token, ';'); solved = (token == "1");
    std::getline(stream, token, ';'); selectedWitness = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';'); successfulInterrogations = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';'); falseAccusations = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';'); lineVariant = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';'); instability = token.empty() ? 0.0f : std::stof(token);

    selectedWitness = std::clamp(selectedWitness, 0, 3);
    lineVariant = std::clamp(lineVariant, 0, 1);
}

void GhostWitnessPuzzle::Reset() {
    selectedWitness = 0;
    successfulInterrogations = 0;
    falseAccusations = 0;
    solved = false;
    ghostPulse = 0.0f;
    instability = 0.0f;
    lineVariant = 0;
    statusLine.clear();
    statusTimer = 0.0f;
}
