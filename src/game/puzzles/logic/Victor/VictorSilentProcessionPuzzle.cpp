#include "game/puzzles/logic/Victor/VictorSilentProcessionPuzzle.h"

#include <algorithm>
#include <sstream>

#include <glm/glm.hpp>

#include "engine/renderer/TextRenderer.h"

VictorSilentProcessionPuzzle::VictorSilentProcessionPuzzle(std::string title, std::string clue, std::string solvedMessage)
    : title(std::move(title)), clue(std::move(clue)), solvedMessage(std::move(solvedMessage)) {
}

void VictorSilentProcessionPuzzle::Start() {
    Reset();
    statusLine = "Do not step into the street. Watch the dead pass by.";
    statusTimer = 3.0f;
    PlaySound("memory_fragment");
}

int VictorSilentProcessionPuzzle::DecodedCount() const {
    return static_cast<int>(std::count(decoded.begin(), decoded.end(), true));
}

void VictorSilentProcessionPuzzle::AdvanceLoop() {
    currentLoop++;
    if (currentLoop > 4) {
        currentLoop = 1;
    }
    darkness = std::min(1.0f, darkness + 0.07f);
    statusLine = "The procession repeats, but one detail changes.";
    statusTimer = 2.0f;
    PlaySound("puzzle_tick");
}

void VictorSilentProcessionPuzzle::Approach(float amount) {
    distanceToProcession = std::clamp(distanceToProcession + amount, 0.0f, 1.0f);
    if (distanceToProcession < 0.22f) {
        noticed = true;
        suspicion++;
        darkness = std::min(1.0f, darkness + 0.22f);
        statusLine = "Every head turns at once. Victor should not be here.";
        statusTimer = 2.8f;
        PlaySound("puzzle_fail");
        if (suspicion >= 3) {
            insertedIntoProcession = true;
            statusLine = "Victor sees himself walking among the dead.";
            statusTimer = 4.0f;
        }
    } else if (amount > 0.0f) {
        statusLine = "Too close. The lanterns slow down.";
        statusTimer = 1.8f;
    } else {
        noticed = false;
        statusLine = "Victor retreats into shadow. The procession resumes.";
        statusTimer = 1.8f;
    }
}

void VictorSilentProcessionPuzzle::MarkPattern(int index) {
    selectedPattern = std::clamp(index, 0, 3);
    if (noticed || insertedIntoProcession) {
        statusLine = "They are watching Victor. He cannot read the pattern now.";
        statusTimer = 2.0f;
        PlaySound("puzzle_locked");
        return;
    }

    if (currentLoop == requiredLoop[static_cast<std::size_t>(selectedPattern)]) {
        decoded[static_cast<std::size_t>(selectedPattern)] = true;
        darkness = std::max(0.0f, darkness - 0.08f);
        statusLine = "Victor records the anomaly without being seen.";
        statusTimer = 2.0f;
        PlaySound("puzzle_tick");
    } else {
        suspicion++;
        darkness = std::min(1.0f, darkness + 0.15f);
        statusLine = "Wrong loop. The procession loses a footstep and notices the gap.";
        statusTimer = 2.3f;
        PlaySound("puzzle_fail");
    }

    if (DecodedCount() >= 4) {
        solved = true;
        solvedMessage = "Victor deciphers the ritual route, the sacrifice order, and the original cult participants.";
        statusLine = "The lantern route draws the cult symbol in the street.";
        statusTimer = 4.0f;
        PlaySound("puzzle_complete");
    }
}

void VictorSilentProcessionPuzzle::Update(float dt, const InputContext& input) {
    loopTimer += dt;
    statusTimer = std::max(0.0f, statusTimer - dt);
    darkness = std::max(0.0f, darkness - dt * 0.01f);

    if (input.IsActionPressed(InputAction::PuzzleReset)) {
        Start();
        return;
    }

    if (solved || insertedIntoProcession) {
        return;
    }

    if (loopTimer >= 6.0f) {
        loopTimer = 0.0f;
        AdvanceLoop();
    }

    if (input.IsActionDown(InputAction::PuzzleMoveForward)) {
        Approach(-dt * 0.34f);
    }
    if (input.IsActionDown(InputAction::PuzzleMoveBackward)) {
        Approach(dt * 0.40f);
    }

    if (input.IsActionPressed(InputAction::PuzzleOption1)) MarkPattern(0);
    if (input.IsActionPressed(InputAction::PuzzleOption2)) MarkPattern(1);
    if (input.IsActionPressed(InputAction::PuzzleOption3)) MarkPattern(2);
    if (input.IsActionPressed(InputAction::PuzzleOption4)) MarkPattern(3);
    if (input.IsActionPressed(InputAction::PuzzleCancel)) {
        Approach(0.25f);
    }
}

void VictorSilentProcessionPuzzle::Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const {
    const float x = 72.0f;
    const float topY = static_cast<float>(screenHeight) * 0.13f;
    const glm::vec3 titleColor(0.76f, 0.84f, 1.0f);
    const glm::vec3 ghostColor(0.72f, 0.92f, 0.82f);
    const glm::vec3 dangerColor(1.0f, 0.62f, 0.42f);
    const glm::vec3 selectedColor(1.0f, 0.92f, 0.44f);

    textRenderer.RenderText(GetTitle(), x, topY, 0.86f, titleColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(title, x, topY + 34.0f, 0.46f, dangerColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(clue, x, topY + 62.0f, 0.42f, ghostColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("Observe from hiding. Do not interact with the procession directly.", x, topY + 94.0f, 0.40f, dangerColor * alpha, screenWidth, screenHeight);

    for (int i = 0; i < 4; ++i) {
        const std::string state = decoded[static_cast<std::size_t>(i)] ? " [decoded]" : "";
        const glm::vec3 color = selectedPattern == i ? selectedColor : ghostColor;
        textRenderer.RenderText(std::to_string(i + 1) + ". " + observations[static_cast<std::size_t>(i)] + state,
                                x, topY + 136.0f + static_cast<float>(i) * 34.0f, 0.35f, color * alpha, screenWidth, screenHeight);
    }

    textRenderer.RenderText("Current nightly loop: " + std::to_string(currentLoop) + " / hidden pattern fragments: " + std::to_string(DecodedCount()) + "/4",
                            x, topY + 292.0f, 0.42f, selectedColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("Distance from procession: " + std::string(static_cast<int>(distanceToProcession * 18.0f), '.') +
                            "  Suspicion: " + std::string(suspicion, '!'),
                            x, topY + 324.0f, 0.40f, ghostColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("Darkness: " + std::string(static_cast<int>(darkness * 18.0f), '#'),
                            x, static_cast<float>(screenHeight) - 128.0f, 0.40f, dangerColor * alpha, screenWidth, screenHeight);

    if (!statusLine.empty() && statusTimer > 0.0f) {
        textRenderer.RenderText(statusLine, x, static_cast<float>(screenHeight) - 92.0f, 0.44f, dangerColor * alpha, screenWidth, screenHeight);
    }
    if (insertedIntoProcession) {
        textRenderer.RenderText("VICTOR HAS BEEN INSERTED INTO THE PROCESSION", x, static_cast<float>(screenHeight) * 0.82f, 0.52f, dangerColor * alpha, screenWidth, screenHeight);
    } else if (solved) {
        textRenderer.RenderText("THE ROUTE REVEALS THE ORIGINAL CULT", x, static_cast<float>(screenHeight) * 0.82f, 0.54f, ghostColor * alpha, screenWidth, screenHeight);
    }

    textRenderer.RenderText("[1-4] mark observed pattern  [W/S] lean closer/back away  [ESC] retreat  [R] restart",
                            x, static_cast<float>(screenHeight) - 42.0f, 0.40f, titleColor * alpha, screenWidth, screenHeight);
}

std::string VictorSilentProcessionPuzzle::SerializeState() const {
    std::ostringstream stream;
    stream << (solved ? 1 : 0) << ';' << selectedPattern << ';' << currentLoop << ';' << suspicion << ';'
           << (noticed ? 1 : 0) << ';' << (insertedIntoProcession ? 1 : 0) << ';' << distanceToProcession << ';' << darkness;
    for (bool mark : decoded) {
        stream << ';' << (mark ? 1 : 0);
    }
    return stream.str();
}

void VictorSilentProcessionPuzzle::DeserializeState(const std::string& state) {
    std::stringstream stream(state);
    std::string token;
    std::getline(stream, token, ';'); solved = (token == "1");
    std::getline(stream, token, ';'); selectedPattern = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';'); currentLoop = token.empty() ? 1 : std::stoi(token);
    std::getline(stream, token, ';'); suspicion = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';'); noticed = (token == "1");
    std::getline(stream, token, ';'); insertedIntoProcession = (token == "1");
    std::getline(stream, token, ';'); distanceToProcession = token.empty() ? 0.62f : std::stof(token);
    std::getline(stream, token, ';'); darkness = token.empty() ? 0.0f : std::stof(token);
    for (bool& mark : decoded) {
        std::getline(stream, token, ';');
        mark = (token == "1");
    }
    selectedPattern = std::clamp(selectedPattern, 0, 3);
    currentLoop = std::clamp(currentLoop, 1, 4);
    distanceToProcession = std::clamp(distanceToProcession, 0.0f, 1.0f);
}

void VictorSilentProcessionPuzzle::Reset() {
    decoded = {{false, false, false, false}};
    selectedPattern = 0;
    currentLoop = 1;
    suspicion = 0;
    noticed = false;
    insertedIntoProcession = false;
    solved = false;
    loopTimer = 0.0f;
    distanceToProcession = 0.62f;
    darkness = 0.0f;
    statusTimer = 0.0f;
    statusLine.clear();
}
