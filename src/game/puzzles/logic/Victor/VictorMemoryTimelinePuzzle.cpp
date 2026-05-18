#include "game/puzzles/logic/Victor/VictorMemoryTimelinePuzzle.h"

#include <algorithm>
#include <sstream>

#include <glm/glm.hpp>

#include "engine/renderer/TextRenderer.h"

VictorMemoryTimelinePuzzle::VictorMemoryTimelinePuzzle(std::string title, std::string clue, std::string solvedMessage)
    : title(std::move(title)), clue(std::move(clue)), solvedMessage(std::move(solvedMessage)) {
}

void VictorMemoryTimelinePuzzle::Start() {
    Reset();
    statusLine = "Someone has been erased. Find the missing person before the memory fills the gap.";
    statusTimer = 2.8f;
    PlaySound("memory_fragment");
}

void VictorMemoryTimelinePuzzle::ShiftMissingPerson() {
    missingIndex = (missingIndex + 1) % static_cast<int>(people.size());
    absencePressure = std::min(1.0f, absencePressure + 0.16f);
    MemoryReplaySystem::Instance().TriggerGhostReplica(people[static_cast<std::size_t>(missingIndex)], "I was here. Then I wasn't.", glm::vec3(0.0f));
    statusLine = "The absence moved. Notice what no longer belongs in the room.";
    statusTimer = 2.0f;
    PlaySound("puzzle_tick");
}

void VictorMemoryTimelinePuzzle::GuessMissing(int index) {
    if (index < 0 || index >= static_cast<int>(people.size())) {
        return;
    }

    if (index == missingIndex) {
        successfulFinds++;
        absencePressure = std::max(0.0f, absencePressure - 0.12f);
        statusLine = people[static_cast<std::size_t>(index)] + " was erased from the room.";
        statusTimer = 2.0f;
        PlaySound("puzzle_tick");
        if (successfulFinds >= 4) {
            solved = true;
            statusLine = "The erased person is remembered back into the world.";
            solvedMessage = "Victor reconstructs the missing identity from its absence.";
            PlaySound("puzzle_complete");
        } else {
            ShiftMissingPerson();
        }
    } else {
        wrongGuesses++;
        absencePressure = std::min(1.0f, absencePressure + 0.18f);
        statusLine = "Wrong absence. The room rejects the guess.";
        statusTimer = 2.0f;
        PlaySound("puzzle_fail");
        MemoryReplaySystem::Instance().IncraseTrauma(0.10f);
    }
}

void VictorMemoryTimelinePuzzle::Update(float dt, const InputContext& input) {
    timer += dt;
    statusTimer = std::max(0.0f, statusTimer - dt);
    absencePressure = std::max(0.0f, absencePressure - dt * 0.04f);

    if (input.IsActionPressed(InputAction::PuzzleReset)) {
        Start();
        return;
    }

    if (solved) {
        return;
    }

    if (timer >= 7.0f) {
        timer = 0.0f;
        ShiftMissingPerson();
    }

    if (input.IsActionPressed(InputAction::PuzzleOption1)) missingIndex = 0;
    if (input.IsActionPressed(InputAction::PuzzleOption2)) missingIndex = 1;
    if (input.IsActionPressed(InputAction::PuzzleOption3)) missingIndex = 2;
    if (input.IsActionPressed(InputAction::PuzzleOption4)) missingIndex = 3;

    if (input.IsActionPressed(InputAction::PuzzleConfirm)) {
        GuessMissing(missingIndex);
    }

    if (input.IsActionPressed(InputAction::PuzzleCancel)) {
        absencePressure = std::min(1.0f, absencePressure + 0.08f);
        statusLine = "The hole where the person used to be starts pulling harder.";
        statusTimer = 1.6f;
        PlaySound("puzzle_tick");
    }
}

void VictorMemoryTimelinePuzzle::Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const {
    const float topY = static_cast<float>(screenHeight) * 0.18f;
    const glm::vec3 titleColor(0.7f, 0.82f, 1.0f);
    const glm::vec3 warnColor(1.0f, 0.8f, 0.48f);
    const glm::vec3 calmColor(0.72f, 0.95f, 0.72f);

    textRenderer.RenderText(GetTitle(), 72.0f, topY, 0.72f, titleColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(title, 72.0f, topY + 34.0f, 0.34f, warnColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(clue, 72.0f, topY + 62.0f, 0.32f, calmColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("Identify the person who is missing from reality, not the order they appeared in.", 72.0f, topY + 96.0f, 0.32f, warnColor * alpha, screenWidth, screenHeight);

    for (int i = 0; i < 4; ++i) {
        const glm::vec3 color = (missingIndex == i) ? glm::vec3(1.0f, 0.95f, 0.5f) : calmColor;
        textRenderer.RenderText(std::to_string(i + 1) + ". " + people[static_cast<std::size_t>(i)] + " - " + clues[static_cast<std::size_t>(i)],
                                72.0f, topY + 140.0f + static_cast<float>(i) * 32.0f, 0.30f, color * alpha, screenWidth, screenHeight);
    }

    textRenderer.RenderText("Missing recovered: " + std::to_string(successfulFinds) + "/4", 72.0f, static_cast<float>(screenHeight) - 160.0f, 0.36f, titleColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("Absence pressure: " + std::string(static_cast<int>(absencePressure * 20.0f), '!'), 72.0f, static_cast<float>(screenHeight) - 128.0f, 0.32f, warnColor * alpha, screenWidth, screenHeight);

    if (!statusLine.empty() && statusTimer > 0.0f) {
        textRenderer.RenderText(statusLine, 72.0f, static_cast<float>(screenHeight) - 92.0f, 0.34f, (solved ? calmColor : warnColor) * alpha, screenWidth, screenHeight);
    }

    if (solved) {
        textRenderer.RenderText("THE ERASED PERSON RETURNS TO MEMORY", 72.0f, static_cast<float>(screenHeight) * 0.82f, 0.42f, calmColor * alpha, screenWidth, screenHeight);
    }

    textRenderer.RenderText("[1-4] select person  [ENTER] test absence  [L] steady  [R] restart", 72.0f, static_cast<float>(screenHeight) - 42.0f, 0.30f, titleColor * alpha, screenWidth, screenHeight);
}

std::string VictorMemoryTimelinePuzzle::SerializeState() const {
    std::ostringstream stream;
    stream << (solved ? 1 : 0) << ';' << missingIndex << ';' << successfulFinds << ';' << wrongGuesses << ';' << timer << ';' << absencePressure;
    return stream.str();
}

void VictorMemoryTimelinePuzzle::DeserializeState(const std::string& state) {
    std::stringstream stream(state);
    std::string token;

    std::getline(stream, token, ';'); solved = (token == "1");
    std::getline(stream, token, ';'); missingIndex = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';'); successfulFinds = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';'); wrongGuesses = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';'); timer = token.empty() ? 0.0f : std::stof(token);
    std::getline(stream, token, ';'); absencePressure = token.empty() ? 0.0f : std::stof(token);

    missingIndex = std::clamp(missingIndex, 0, 3);
}

void VictorMemoryTimelinePuzzle::Reset() {
    missingIndex = 0;
    successfulFinds = 0;
    wrongGuesses = 0;
    solved = false;
    timer = 0.0f;
    absencePressure = 0.0f;
    statusLine.clear();
    statusTimer = 0.0f;
}
