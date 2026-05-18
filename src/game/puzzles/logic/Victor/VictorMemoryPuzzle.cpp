#include "game/puzzles/logic/Victor/VictorMemoryPuzzle.h"

#include <algorithm>
#include <sstream>

#include <glm/glm.hpp>

#include "engine/renderer/TextRenderer.h"

VictorMemoryPuzzle::VictorMemoryPuzzle(std::string title, std::string clue, std::string solvedMessage)
    : title(std::move(title)), clue(std::move(clue)), solvedMessage(std::move(solvedMessage)) {
}

void VictorMemoryPuzzle::Start() {
    Reset();
    statusLine = "The memory loops. One detail keeps changing.";
    statusTimer = 2.5f;
    PlaySound("memory_fragment");
}

void VictorMemoryPuzzle::CorruptLoop() {
    corruptedScene = (corruptedScene + 1) % static_cast<int>(scenes.size());
    trauma = std::min(1.0f, trauma + 0.18f);
    MemoryReplaySystem::Instance().DistortMemory(scenes[static_cast<std::size_t>(corruptedScene)]);
    statusLine = "The loop changed. Find the corrupted scene.";
    statusTimer = 2.0f;
    PlaySound("puzzle_tick");
}

void VictorMemoryPuzzle::InspectScene(int index) {
    if (index == corruptedScene) {
        detectedCorruptions++;
        trauma = std::max(0.0f, trauma - 0.10f);
        statusLine = "That scene is the distortion. The loop yields.";
        statusTimer = 2.0f;
        PlaySound("puzzle_tick");
        if (detectedCorruptions >= 4) {
            solved = true;
            statusLine = "The memory stops lying. Victor sees the pattern.";
            solvedMessage = "Victor reconstructs the corruption and breaks the loop.";
            PlaySound("puzzle_complete");
        } else {
            CorruptLoop();
        }
    } else {
        wrongCalls++;
        trauma = std::min(1.0f, trauma + 0.16f);
        statusLine = "Wrong scene. The memory bites back.";
        statusTimer = 2.0f;
        PlaySound("puzzle_fail");
        MemoryReplaySystem::Instance().IncraseTrauma(0.12f);
    }
}

void VictorMemoryPuzzle::Update(float dt, const InputContext& input) {
    loopTimer += dt;
    statusTimer = std::max(0.0f, statusTimer - dt);
    trauma = std::max(0.0f, trauma - dt * 0.03f);

    if (input.IsActionPressed(InputAction::PuzzleReset)) {
        Start();
        return;
    }

    if (solved) {
        return;
    }

    if (loopTimer >= 6.0f) {
        loopTimer = 0.0f;
        CorruptLoop();
    }

    if (input.IsActionPressed(InputAction::PuzzleOption1)) corruptedScene = 0;
    if (input.IsActionPressed(InputAction::PuzzleOption2)) corruptedScene = 1;
    if (input.IsActionPressed(InputAction::PuzzleOption3)) corruptedScene = 2;
    if (input.IsActionPressed(InputAction::PuzzleOption4)) corruptedScene = 3;

    if (input.IsActionPressed(InputAction::PuzzleConfirm)) {
        InspectScene(corruptedScene);
    }

    if (input.IsActionPressed(InputAction::PuzzleCancel)) {
        trauma = std::min(1.0f, trauma + 0.08f);
        statusLine = "Victor tries to hold the scene in place, but the corruption keeps moving.";
        statusTimer = 1.8f;
        PlaySound("puzzle_tick");
    }
}

void VictorMemoryPuzzle::Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const {
    const float topY = static_cast<float>(screenHeight) * 0.18f;
    const glm::vec3 titleColor(0.78f, 0.9f, 1.0f);
    const glm::vec3 warnColor(1.0f, 0.8f, 0.45f);
    const glm::vec3 calmColor(0.72f, 0.95f, 0.72f);

    textRenderer.RenderText(GetTitle(), 72.0f, topY, 0.88f, titleColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(title, 72.0f, topY + 34.0f, 0.46f, warnColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(clue, 72.0f, topY + 62.0f, 0.42f, calmColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("One memory detail shifts every loop. Pick the corrupted scene.", 72.0f, topY + 96.0f, 0.42f, warnColor * alpha, screenWidth, screenHeight);

    for (int i = 0; i < 4; ++i) {
        const glm::vec3 color = (corruptedScene == i) ? glm::vec3(1.0f, 0.95f, 0.5f) : calmColor;
        textRenderer.RenderText(std::to_string(i + 1) + ". " + scenes[static_cast<std::size_t>(i)],
                                72.0f, topY + 140.0f + static_cast<float>(i) * 30.0f, 0.40f, color * alpha, screenWidth, screenHeight);
    }

    textRenderer.RenderText("Corruptions found: " + std::to_string(detectedCorruptions) + "/4", 72.0f, static_cast<float>(screenHeight) - 160.0f, 0.46f, titleColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("Trauma: " + std::string(static_cast<int>(trauma * 20.0f), '!'), 72.0f, static_cast<float>(screenHeight) - 128.0f, 0.42f, warnColor * alpha, screenWidth, screenHeight);

    if (!statusLine.empty() && statusTimer > 0.0f) {
        textRenderer.RenderText(statusLine, 72.0f, static_cast<float>(screenHeight) - 92.0f, 0.46f, warnColor * alpha, screenWidth, screenHeight);
    }

    if (solved) {
        textRenderer.RenderText("THE MEMORY LOOP STOPS LYING", 72.0f, static_cast<float>(screenHeight) * 0.82f, 0.54f, calmColor * alpha, screenWidth, screenHeight);
    }

    textRenderer.RenderText("[1-4] choose scene  [ENTER] inspect  [L] steady  [R] restart", 72.0f, static_cast<float>(screenHeight) - 42.0f, 0.42f, titleColor * alpha, screenWidth, screenHeight);
}

std::string VictorMemoryPuzzle::SerializeState() const {
    std::ostringstream stream;
    stream << (solved ? 1 : 0) << ';' << corruptedScene << ';' << detectedCorruptions << ';' << wrongCalls << ';' << loopTimer << ';' << trauma;
    return stream.str();
}

void VictorMemoryPuzzle::DeserializeState(const std::string& state) {
    std::stringstream stream(state);
    std::string token;

    std::getline(stream, token, ';'); solved = (token == "1");
    std::getline(stream, token, ';'); corruptedScene = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';'); detectedCorruptions = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';'); wrongCalls = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';'); loopTimer = token.empty() ? 0.0f : std::stof(token);
    std::getline(stream, token, ';'); trauma = token.empty() ? 0.0f : std::stof(token);

    corruptedScene = std::clamp(corruptedScene, 0, 3);
}

void VictorMemoryPuzzle::Reset() {
    corruptedScene = 0;
    detectedCorruptions = 0;
    wrongCalls = 0;
    solved = false;
    loopTimer = 0.0f;
    trauma = 0.0f;
    statusLine.clear();
    statusTimer = 0.0f;
}
