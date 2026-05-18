#include "game/puzzles/logic/Victor/VictorEmptyChairPuzzle.h"

#include <algorithm>
#include <sstream>

#include <glm/glm.hpp>

#include "engine/renderer/TextRenderer.h"

VictorEmptyChairPuzzle::VictorEmptyChairPuzzle(std::string title, std::string clue, std::string solvedMessage)
    : title(std::move(title)), clue(std::move(clue)), solvedMessage(std::move(solvedMessage)) {
}

void VictorEmptyChairPuzzle::Start() {
    Reset();
    statusLine = "The room has a place for someone the world refuses to remember.";
    statusTimer = 3.0f;
    PlaySound("memory_fragment");
}

int VictorEmptyChairPuzzle::ObservationCount() const {
    return static_cast<int>(std::count(observed.begin(), observed.end(), true));
}

void VictorEmptyChairPuzzle::RewriteRoom() {
    rewritePhase = 1 - rewritePhase;
    instability = std::min(1.0f, instability + 0.11f);
    statusLine = "Reality edits the room while Victor is looking at it.";
    statusTimer = 2.2f;
    PlaySound("puzzle_tick");
}

void VictorEmptyChairPuzzle::Observe(int index) {
    selectedEvidence = std::clamp(index, 0, 3);
    observed[static_cast<std::size_t>(selectedEvidence)] = true;
    instability = std::max(0.0f, instability - 0.05f);
    statusLine = "Victor marks an absence. The room pushes back.";
    statusTimer = 2.0f;
    PlaySound("puzzle_tick");
}

void VictorEmptyChairPuzzle::Reconstruct() {
    if (ObservationCount() < 3) {
        statusLine = "Too many gaps. The missing person has no shape yet.";
        statusTimer = 2.2f;
        instability = std::min(1.0f, instability + 0.12f);
        PlaySound("puzzle_locked");
        return;
    }

    if (selectedIdentity == 0) {
        solved = true;
        solvedMessage = "Clara partially manifests beside the empty chair. Victor remembers who the town erased.";
        statusLine = "The chair stops rocking. Someone breathes where nobody was.";
        statusTimer = 4.0f;
        PlaySound("puzzle_complete");
        return;
    }

    wrongReconstructions++;
    instability = std::min(1.0f, instability + 0.24f);
    statusLine = "Wrong name. Victor forgets the color of his own coat.";
    statusTimer = 2.8f;
    PlaySound("puzzle_fail");
    RewriteRoom();
}

void VictorEmptyChairPuzzle::Update(float dt, const InputContext& input) {
    rewriteTimer += dt;
    statusTimer = std::max(0.0f, statusTimer - dt);
    instability = std::max(0.0f, instability - dt * 0.015f);

    if (input.IsActionPressed(InputAction::PuzzleReset)) {
        Start();
        return;
    }

    if (solved) {
        return;
    }

    if (rewriteTimer >= 5.5f) {
        rewriteTimer = 0.0f;
        RewriteRoom();
    }

    if (input.IsActionPressed(InputAction::PuzzleOption1)) Observe(0);
    if (input.IsActionPressed(InputAction::PuzzleOption2)) Observe(1);
    if (input.IsActionPressed(InputAction::PuzzleOption3)) Observe(2);
    if (input.IsActionPressed(InputAction::PuzzleOption4)) Observe(3);

    if (input.IsActionPressed(InputAction::PuzzleDecrease)) {
        selectedIdentity = (selectedIdentity + 3) % 4;
        PlaySound("puzzle_tick");
    }
    if (input.IsActionPressed(InputAction::PuzzleIncrease)) {
        selectedIdentity = (selectedIdentity + 1) % 4;
        PlaySound("puzzle_tick");
    }
    if (input.IsActionPressed(InputAction::PuzzleConfirm)) {
        Reconstruct();
    }
    if (input.IsActionPressed(InputAction::PuzzleCancel)) {
        RewriteRoom();
    }
}

void VictorEmptyChairPuzzle::Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const {
    const float x = 72.0f;
    const float topY = static_cast<float>(screenHeight) * 0.14f;
    const glm::vec3 titleColor(0.80f, 0.88f, 1.0f);
    const glm::vec3 textColor(0.82f, 0.95f, 0.82f);
    const glm::vec3 dangerColor(1.0f, 0.55f, 0.45f);
    const glm::vec3 selectedColor(1.0f, 0.92f, 0.45f);

    textRenderer.RenderText(GetTitle(), x, topY, 0.88f, titleColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(title, x, topY + 34.0f, 0.46f, dangerColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(clue, x, topY + 62.0f, 0.42f, textColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("Observe contradictions, then name the erased person.", x, topY + 94.0f, 0.42f, dangerColor * alpha, screenWidth, screenHeight);

    for (int i = 0; i < 4; ++i) {
        const bool isSelected = selectedEvidence == i;
        const bool isObserved = observed[static_cast<std::size_t>(i)];
        const std::string prefix = std::to_string(i + 1) + (isObserved ? ". [seen] " : ". ");
        const std::string& line = rewritePhase == 0 ? evidence[static_cast<std::size_t>(i)] : rewrittenEvidence[static_cast<std::size_t>(i)];
        textRenderer.RenderText(prefix + line, x, topY + 136.0f + static_cast<float>(i) * 32.0f, 0.36f,
                                (isSelected ? selectedColor : textColor) * alpha, screenWidth, screenHeight);
    }

    textRenderer.RenderText("Identity reconstruction: " + identities[static_cast<std::size_t>(selectedIdentity)],
                            x, topY + 286.0f, 0.44f, selectedColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("Observed absences: " + std::to_string(ObservationCount()) + "/4", x, topY + 318.0f, 0.40f, textColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("Instability: " + std::string(static_cast<int>(instability * 18.0f), '#'),
                            x, static_cast<float>(screenHeight) - 128.0f, 0.40f, dangerColor * alpha, screenWidth, screenHeight);

    if (!statusLine.empty() && statusTimer > 0.0f) {
        textRenderer.RenderText(statusLine, x, static_cast<float>(screenHeight) - 92.0f, 0.44f, dangerColor * alpha, screenWidth, screenHeight);
    }
    if (solved) {
        textRenderer.RenderText("THE ERASED PERSON BREATHES AGAIN", x, static_cast<float>(screenHeight) * 0.82f, 0.54f, textColor * alpha, screenWidth, screenHeight);
    }

    textRenderer.RenderText("[1-4] observe absence  [A/D] choose identity  [ENTER] reconstruct  [L] force rewrite  [R] restart",
                            x, static_cast<float>(screenHeight) - 42.0f, 0.38f, titleColor * alpha, screenWidth, screenHeight);
}

std::string VictorEmptyChairPuzzle::SerializeState() const {
    std::ostringstream stream;
    stream << (solved ? 1 : 0) << ';' << selectedEvidence << ';' << selectedIdentity << ';' << rewritePhase << ';'
           << wrongReconstructions << ';' << instability;
    for (bool seen : observed) {
        stream << ';' << (seen ? 1 : 0);
    }
    return stream.str();
}

void VictorEmptyChairPuzzle::DeserializeState(const std::string& state) {
    std::stringstream stream(state);
    std::string token;
    std::getline(stream, token, ';'); solved = (token == "1");
    std::getline(stream, token, ';'); selectedEvidence = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';'); selectedIdentity = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';'); rewritePhase = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';'); wrongReconstructions = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';'); instability = token.empty() ? 0.0f : std::stof(token);
    for (bool& seen : observed) {
        std::getline(stream, token, ';');
        seen = (token == "1");
    }
    selectedEvidence = std::clamp(selectedEvidence, 0, 3);
    selectedIdentity = std::clamp(selectedIdentity, 0, 3);
    rewritePhase = std::clamp(rewritePhase, 0, 1);
}

void VictorEmptyChairPuzzle::Reset() {
    observed = {{false, false, false, false}};
    selectedEvidence = 0;
    selectedIdentity = 0;
    rewritePhase = 0;
    wrongReconstructions = 0;
    solved = false;
    rewriteTimer = 0.0f;
    instability = 0.0f;
    statusTimer = 0.0f;
    statusLine.clear();
}
