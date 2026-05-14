#include "game/puzzles/logic/Sara/SaraRedemptionPuzzle.h"

#include <algorithm>
#include <sstream>

#include <glm/glm.hpp>

#include "engine/renderer/TextRenderer.h"

SaraRedemptionPuzzle::SaraRedemptionPuzzle(std::string title, std::string clue, std::string solvedMessage)
    : title(std::move(title)), clue(std::move(clue)), solvedMessage(std::move(solvedMessage)) {
}

void SaraRedemptionPuzzle::Start() {
    Reset();
    statusLine = "Stay still and listen. The confession is hiding inside the fear.";
    statusTimer = 2.8f;
    PlaySound("puzzle_open");
}

void SaraRedemptionPuzzle::RecoverFragment() {
    fragmentsRecovered++;
    calm = std::min(1.0f, calm + 0.16f);
    noise = std::max(0.0f, noise - 0.12f);
    MoralCorruptionSystem::Instance().OverhearConversation(suspects[static_cast<std::size_t>(selectedSuspect)].name, "A fragment of guilt slips out.");
    statusLine = "A fragment of the confession surfaces.";
    statusTimer = 1.8f;
    PlaySound("puzzle_tick");

    if (fragmentsRecovered >= 4) {
        phase = Phase::Analysis;
        statusLine = "Enough fragments recovered. Read the fear in the room.";
        statusTimer = 2.4f;
    }
}

void SaraRedemptionPuzzle::AnalyzeSuspect(int index) {
    if (index < 0 || index >= static_cast<int>(suspects.size())) {
        return;
    }

    selectedSuspect = index;
    fearReadings++;
    MoralCorruptionSystem::Instance().DetectGuilt(suspects[static_cast<std::size_t>(index)].name, static_cast<float>(suspects[static_cast<std::size_t>(index)].guilt) * 0.2f);
    statusLine = suspects[static_cast<std::size_t>(index)].name + " is showing " + suspects[static_cast<std::size_t>(index)].bodyLanguage + ".";
    statusTimer = 1.8f;
    PlaySound("puzzle_tick");

    if (fearReadings >= 3) {
        phase = Phase::Judgment;
        statusLine = "You know who collaborated. Choose whether to expose or bury it.";
        statusTimer = 2.2f;
    }
}

void SaraRedemptionPuzzle::CommitJudgment(int index) {
    const int guiltyIndex = 3;
    if (index == guiltyIndex) {
        solved = true;
        solvedMessage = "Sara identifies the collaborator hidden behind the fear.";
        statusLine = "The hidden confession and the fear pattern finally align.";
        statusTimer = 2.8f;
        PlaySound("puzzle_complete");
    } else {
        noise = std::min(1.0f, noise + 0.2f);
        calm = std::max(0.0f, calm - 0.1f);
        statusLine = "Wrong suspect. The room tightens around the mistake.";
        statusTimer = 2.0f;
        PlaySound("puzzle_fail");
    }
}

void SaraRedemptionPuzzle::Update(float dt, const InputContext& input) {
    timer += dt;
    statusTimer = std::max(0.0f, statusTimer - dt);
    noise = std::max(0.0f, noise - dt * 0.02f);

    if (input.IsActionPressed(InputAction::PuzzleReset)) {
        Start();
        return;
    }

    if (solved) {
        return;
    }

    if (phase == Phase::Listening) {
        const bool moving = input.IsActionDown(InputAction::PuzzleMoveForward) || input.IsActionDown(InputAction::PuzzleMoveLeft) ||
                            input.IsActionDown(InputAction::PuzzleMoveBackward) || input.IsActionDown(InputAction::PuzzleMoveRight) ||
                            input.IsActionDown(InputAction::PuzzleSprint);
        if (moving) {
            noise = std::min(1.0f, noise + dt * 0.20f);
            calm = std::max(0.0f, calm - dt * 0.14f);
        } else {
            noise = std::max(0.0f, noise - dt * 0.12f);
            calm = std::min(1.0f, calm + dt * 0.18f);
        }

        if (noise >= 1.0f) {
            statusLine = "Too much noise. The confession vanishes.";
            statusTimer = 2.0f;
            PlaySound("puzzle_fail");
            noise = 0.7f;
            calm = 0.0f;
            return;
        }

        if (calm >= 0.9f) {
            RecoverFragment();
        }
    } else if (phase == Phase::Analysis) {
        if (input.IsActionPressed(InputAction::PuzzleOption1)) AnalyzeSuspect(0);
        if (input.IsActionPressed(InputAction::PuzzleOption2)) AnalyzeSuspect(1);
        if (input.IsActionPressed(InputAction::PuzzleOption3)) AnalyzeSuspect(2);
        if (input.IsActionPressed(InputAction::PuzzleOption4)) AnalyzeSuspect(3);
        if (input.IsActionPressed(InputAction::PuzzleConfirm)) {
            phase = Phase::Judgment;
            statusLine = "The fear profile is clear. Commit to the accusation.";
            statusTimer = 2.0f;
        }
    } else if (phase == Phase::Judgment) {
        if (input.IsActionPressed(InputAction::PuzzleOption1)) CommitJudgment(0);
        if (input.IsActionPressed(InputAction::PuzzleOption2)) CommitJudgment(1);
        if (input.IsActionPressed(InputAction::PuzzleOption3)) CommitJudgment(2);
        if (input.IsActionPressed(InputAction::PuzzleOption4)) CommitJudgment(3);
    }

    if (input.IsActionPressed(InputAction::PuzzleCancel)) {
        noise = std::min(1.0f, noise + 0.08f);
        statusLine = "A footstep ruins the thread of the conversation.";
        statusTimer = 1.6f;
        PlaySound("puzzle_tick");
    }
}

void SaraRedemptionPuzzle::Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const {
    const float topY = static_cast<float>(screenHeight) * 0.18f;
    const glm::vec3 titleColor(1.0f, 0.76f, 0.76f);
    const glm::vec3 warnColor(1.0f, 0.8f, 0.48f);
    const glm::vec3 calmColor(0.8f, 0.95f, 1.0f);

    textRenderer.RenderText(GetTitle(), 72.0f, topY, 0.72f, titleColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(title, 72.0f, topY + 34.0f, 0.34f, warnColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(clue, 72.0f, topY + 62.0f, 0.32f, calmColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("Recover fragments, then read who is lying through fear.", 72.0f, topY + 96.0f, 0.32f, warnColor * alpha, screenWidth, screenHeight);

    const char* phaseText = "PHASE: UNKNOWN";
    switch (phase) {
        case Phase::Listening: phaseText = "PHASE: LISTENING"; break;
        case Phase::Analysis: phaseText = "PHASE: ANALYSIS"; break;
        case Phase::Judgment: phaseText = "PHASE: JUDGMENT"; break;
    }
    textRenderer.RenderText(phaseText, 72.0f, topY + 128.0f, 0.36f, warnColor * alpha, screenWidth, screenHeight);

    textRenderer.RenderText("Fragments recovered: " + std::to_string(fragmentsRecovered) + "/4", 72.0f, static_cast<float>(screenHeight) - 164.0f, 0.36f, titleColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("Noise: " + std::string(static_cast<int>(noise * 20.0f), '!'), 72.0f, static_cast<float>(screenHeight) - 132.0f, 0.32f, warnColor * alpha, screenWidth, screenHeight);

    if (phase != Phase::Listening) {
        for (int i = 0; i < 4; ++i) {
            const glm::vec3 color = (selectedSuspect == i) ? glm::vec3(1.0f, 0.95f, 0.5f) : calmColor;
            textRenderer.RenderText(std::to_string(i + 1) + ". " + suspects[static_cast<std::size_t>(i)].name + " - " + suspects[static_cast<std::size_t>(i)].bodyLanguage,
                                    72.0f, topY + 172.0f + static_cast<float>(i) * 30.0f, 0.30f, color * alpha, screenWidth, screenHeight);
        }
    }

    if (!statusLine.empty() && statusTimer > 0.0f) {
        textRenderer.RenderText(statusLine, 72.0f, static_cast<float>(screenHeight) - 96.0f, 0.34f, (solved ? calmColor : warnColor) * alpha, screenWidth, screenHeight);
    }

    if (solved) {
        textRenderer.RenderText("THE HIDDEN CONFESSION HAS BEEN HEARD", 72.0f, static_cast<float>(screenHeight) * 0.82f, 0.42f, calmColor * alpha, screenWidth, screenHeight);
    }

    textRenderer.RenderText("[MOVE] stay quiet  [1-4] analyze/accuse  [ENTER] advance  [ESC] steady  [R] restart", 72.0f, static_cast<float>(screenHeight) - 42.0f, 0.29f, titleColor * alpha, screenWidth, screenHeight);
}

std::string SaraRedemptionPuzzle::SerializeState() const {
    std::ostringstream stream;
    stream << (solved ? 1 : 0) << ';' << static_cast<int>(phase) << ';' << selectedSuspect << ';' << fragmentsRecovered << ';'
           << fearReadings << ';' << noise << ';' << calm << ';' << timer;
    return stream.str();
}

void SaraRedemptionPuzzle::DeserializeState(const std::string& state) {
    std::stringstream stream(state);
    std::string token;

    std::getline(stream, token, ';'); solved = (token == "1");
    std::getline(stream, token, ';'); phase = static_cast<Phase>(token.empty() ? 0 : std::stoi(token));
    std::getline(stream, token, ';'); selectedSuspect = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';'); fragmentsRecovered = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';'); fearReadings = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';'); noise = token.empty() ? 0.0f : std::stof(token);
    std::getline(stream, token, ';'); calm = token.empty() ? 0.0f : std::stof(token);
    std::getline(stream, token, ';'); timer = token.empty() ? 0.0f : std::stof(token);

    selectedSuspect = std::clamp(selectedSuspect, 0, 3);
}

void SaraRedemptionPuzzle::Reset() {
    phase = Phase::Listening;
    selectedSuspect = 0;
    fragmentsRecovered = 0;
    fearReadings = 0;
    solved = false;
    noise = 0.0f;
    calm = 0.0f;
    timer = 0.0f;
    statusLine.clear();
    statusTimer = 0.0f;
}
