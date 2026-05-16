#include "game/puzzles/logic/Victor/VictorMirrorCollapsePuzzle.h"

#include <algorithm>
#include <sstream>

#include <glm/glm.hpp>

#include "engine/renderer/TextRenderer.h"

VictorMirrorCollapsePuzzle::VictorMirrorCollapsePuzzle(std::string title, std::string clue, std::string solvedMessage)
    : title(std::move(title)), clue(std::move(clue)), solvedMessage(std::move(solvedMessage)) {
}

void VictorMirrorCollapsePuzzle::Start() {
    Reset();
    statusLine = "Every mirror offers a memory. Some are bait.";
    statusTimer = 3.0f;
    PlaySound("memory_fragment");
}

int VictorMirrorCollapsePuzzle::DecisionsMade() const {
    return static_cast<int>(std::count(decided.begin(), decided.end(), true));
}

void VictorMirrorCollapsePuzzle::ShiftHouse() {
    layoutState = (layoutState + 1) % 4;
    sanityDecay = std::min(1.0f, sanityDecay + 0.14f);
    statusLine = "The house accepts the false memory and moves a hallway.";
    statusTimer = 2.3f;
    PlaySound("puzzle_fail");
}

void VictorMirrorCollapsePuzzle::InspectMirror(int index) {
    selectedMirror = std::clamp(index, 0, 3);
    statusLine = "Victor compares the reflection against the room itself.";
    statusTimer = 1.8f;
    PlaySound("puzzle_tick");
}

void VictorMirrorCollapsePuzzle::TrustSelected(bool trust) {
    if (decided[static_cast<std::size_t>(selectedMirror)]) {
        statusLine = "That mirror has already chosen what it wants to be.";
        statusTimer = 1.8f;
        PlaySound("puzzle_locked");
        return;
    }

    decided[static_cast<std::size_t>(selectedMirror)] = true;
    trusted[static_cast<std::size_t>(selectedMirror)] = trust;

    if (trust == realMemory[static_cast<std::size_t>(selectedMirror)]) {
        truthRecovered++;
        sanityDecay = std::max(0.0f, sanityDecay - 0.10f);
        statusLine = trust ? "The true reflection cracks at the edges." : "Victor rejects the mirror that was wearing someone else's grief.";
        statusTimer = 2.4f;
        PlaySound("puzzle_tick");
    } else {
        falseLoopDepth++;
        ShiftHouse();
    }

    if (truthRecovered >= 4) {
        solved = true;
        solvedMessage = "The mirrors shatter in order. Victor keeps only the memories that match the impossible house.";
        statusLine = "The false rooms fold inward. The true memory remains.";
        statusTimer = 4.0f;
        PlaySound("puzzle_complete");
    }
}

void VictorMirrorCollapsePuzzle::Update(float dt, const InputContext& input) {
    flickerTimer += dt;
    statusTimer = std::max(0.0f, statusTimer - dt);
    sanityDecay = std::max(0.0f, sanityDecay - dt * 0.012f);

    if (input.IsActionPressed(InputAction::PuzzleReset)) {
        Start();
        return;
    }

    if (solved) {
        return;
    }

    if (flickerTimer >= 4.0f) {
        flickerTimer = 0.0f;
        layoutState = (layoutState + 1) % 4;
        statusLine = "A mirror shows the hallway from the wrong side.";
        statusTimer = 1.7f;
    }

    if (input.IsActionPressed(InputAction::PuzzleOption1)) InspectMirror(0);
    if (input.IsActionPressed(InputAction::PuzzleOption2)) InspectMirror(1);
    if (input.IsActionPressed(InputAction::PuzzleOption3)) InspectMirror(2);
    if (input.IsActionPressed(InputAction::PuzzleOption4)) InspectMirror(3);
    if (input.IsActionPressed(InputAction::PuzzleConfirm)) TrustSelected(true);
    if (const InputManager* inputManager = input.GetInputManager()) {
        if (inputManager->IsKeyPressed(GLFW_KEY_X)) {
            TrustSelected(false);
        }
    }
}

void VictorMirrorCollapsePuzzle::Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const {
    const float x = 72.0f;
    const float topY = static_cast<float>(screenHeight) * 0.13f;
    const glm::vec3 titleColor(0.78f, 0.88f, 1.0f);
    const glm::vec3 mirrorColor(0.72f, 0.95f, 1.0f);
    const glm::vec3 dangerColor(1.0f, 0.55f, 0.62f);
    const glm::vec3 selectedColor(1.0f, 0.92f, 0.44f);

    textRenderer.RenderText(GetTitle(), x, topY, 0.86f, titleColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(title, x, topY + 34.0f, 0.46f, dangerColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(clue, x, topY + 62.0f, 0.42f, mirrorColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("Trust only reflections that agree emotionally and physically with the house.", x, topY + 94.0f, 0.40f, dangerColor * alpha, screenWidth, screenHeight);

    for (int i = 0; i < 4; ++i) {
        std::string state = "";
        if (decided[static_cast<std::size_t>(i)]) {
            state = trusted[static_cast<std::size_t>(i)] ? " [trusted]" : " [rejected]";
        }
        const glm::vec3 color = selectedMirror == i ? selectedColor : mirrorColor;
        textRenderer.RenderText(std::to_string(i + 1) + ". " + mirrors[static_cast<std::size_t>(i)] + state,
                                x, topY + 136.0f + static_cast<float>(i) * 34.0f, 0.35f, color * alpha, screenWidth, screenHeight);
    }

    textRenderer.RenderText("Physical clue: " + physicalClues[static_cast<std::size_t>(selectedMirror)],
                            x, topY + 292.0f, 0.40f, selectedColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("House layout: " + std::to_string(layoutState + 1) + " / decisions: " + std::to_string(DecisionsMade()) + "/4",
                            x, topY + 324.0f, 0.40f, mirrorColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("False loop depth: " + std::string(falseLoopDepth, '@') + "  Sanity decay: " + std::string(static_cast<int>(sanityDecay * 16.0f), '!'),
                            x, static_cast<float>(screenHeight) - 128.0f, 0.40f, dangerColor * alpha, screenWidth, screenHeight);

    if (!statusLine.empty() && statusTimer > 0.0f) {
        textRenderer.RenderText(statusLine, x, static_cast<float>(screenHeight) - 92.0f, 0.44f, dangerColor * alpha, screenWidth, screenHeight);
    }
    if (solved) {
        textRenderer.RenderText("THE FALSE REFLECTIONS SHATTER", x, static_cast<float>(screenHeight) * 0.82f, 0.54f, mirrorColor * alpha, screenWidth, screenHeight);
    }

    textRenderer.RenderText("[1-4] inspect mirror  [ENTER] trust  [X] reject  [R] restart  [ESC] close puzzle",
                            x, static_cast<float>(screenHeight) - 42.0f, 0.40f, titleColor * alpha, screenWidth, screenHeight);
}

std::string VictorMirrorCollapsePuzzle::SerializeState() const {
    std::ostringstream stream;
    stream << (solved ? 1 : 0) << ';' << selectedMirror << ';' << truthRecovered << ';' << falseLoopDepth << ';'
           << layoutState << ';' << sanityDecay;
    for (int i = 0; i < 4; ++i) {
        stream << ';' << (trusted[static_cast<std::size_t>(i)] ? 1 : 0)
               << ';' << (decided[static_cast<std::size_t>(i)] ? 1 : 0);
    }
    return stream.str();
}

void VictorMirrorCollapsePuzzle::DeserializeState(const std::string& state) {
    std::stringstream stream(state);
    std::string token;
    std::getline(stream, token, ';'); solved = (token == "1");
    std::getline(stream, token, ';'); selectedMirror = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';'); truthRecovered = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';'); falseLoopDepth = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';'); layoutState = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';'); sanityDecay = token.empty() ? 0.0f : std::stof(token);
    for (int i = 0; i < 4; ++i) {
        std::getline(stream, token, ';'); trusted[static_cast<std::size_t>(i)] = (token == "1");
        std::getline(stream, token, ';'); decided[static_cast<std::size_t>(i)] = (token == "1");
    }
    selectedMirror = std::clamp(selectedMirror, 0, 3);
    layoutState = std::clamp(layoutState, 0, 3);
}

void VictorMirrorCollapsePuzzle::Reset() {
    trusted = {{false, false, false, false}};
    decided = {{false, false, false, false}};
    selectedMirror = 0;
    truthRecovered = 0;
    falseLoopDepth = 0;
    layoutState = 0;
    solved = false;
    flickerTimer = 0.0f;
    sanityDecay = 0.0f;
    statusTimer = 0.0f;
    statusLine.clear();
}
