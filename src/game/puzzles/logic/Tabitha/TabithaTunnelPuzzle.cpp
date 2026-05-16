#include "game/puzzles/logic/Tabitha/TabithaTunnelPuzzle.h"

#include <algorithm>
#include <sstream>
#include <glm/glm.hpp>

#include "engine/renderer/TextRenderer.h"
#include "engine/input/InputContext.h"
#include "game/tunnels/TunnelMappingSystem.h"

TabithaTunnelPuzzle::TabithaTunnelPuzzle(std::string title, std::string clue, std::string solvedMessage)
    : title(std::move(title)), clue(std::move(clue)), solvedMessage(std::move(solvedMessage)) {
}

void TabithaTunnelPuzzle::Start() {
    Reset();
    statusLine = "Listen for the survivor who breathes wrong.";
    statusTimer = 2.5f;
}

void TabithaTunnelPuzzle::AdvanceSample() {
    currentSample = (currentSample + 1) % static_cast<int>(samples.size());
    sampleTimer = 0.0f;
    statusLine = "Another voice crawls through the tunnel walls.";
    statusTimer = 2.0f;
}

int TabithaTunnelPuzzle::MimicCount() const {
    int count = 0;
    for (const VoiceSample& sample : samples) {
        if (sample.mimic) {
            ++count;
        }
    }
    return count;
}

void TabithaTunnelPuzzle::EvaluateSuspect(int index) {
    if (index < 0 || index >= static_cast<int>(samples.size())) return;

    selectedSuspect = index;
    if (identified[static_cast<std::size_t>(index)]) {
        statusLine = "You already marked that voice. Listen for another false breath.";
        statusTimer = 2.0f;
        return;
    }

    if (samples[static_cast<std::size_t>(index)].mimic) {
        identified[static_cast<std::size_t>(index)] = true;
        correctIdentifications++;
        paranoia = std::max(0.0f, paranoia - 0.15f);
        statusLine = "That breath pattern is wrong. The mimic is exposed.";
        statusTimer = 2.2f;
        TunnelMappingSystem::Instance().PlayDistortedEcho();
        if (correctIdentifications >= MimicCount()) {
            solved = true;
            statusLine = "The mimic loses the survivors' voices.";
            solvedMessage = "Tabitha isolates the false voices without accusing every survivor.";
        } else {
            AdvanceSample();
        }
    } else {
        falseCalls++;
        paranoia = std::min(1.0f, paranoia + 0.22f);
        statusLine = "Wrong target. The real survivor would never sound like that.";
        statusTimer = 2.1f;
        TunnelMappingSystem::Instance().TriggerTunnelShift();
    }
}

void TabithaTunnelPuzzle::Update(float dt, const InputContext& input) {
    sampleTimer += dt;
    statusTimer = std::max(0.0f, statusTimer - dt);
    paranoia = std::max(0.0f, paranoia - dt * 0.05f);

    if (solved) return;

    if (sampleTimer >= 5.5f) {
        AdvanceSample();
    }

    if (input.IsActionPressed(InputAction::PuzzleOption1)) selectedSuspect = 0;
    if (input.IsActionPressed(InputAction::PuzzleOption2)) selectedSuspect = 1;
    if (input.IsActionPressed(InputAction::PuzzleOption3)) selectedSuspect = 2;
    if (input.IsActionPressed(InputAction::PuzzleOption4)) selectedSuspect = 3;

    if (input.IsActionPressed(InputAction::PuzzleConfirm)) {
        EvaluateSuspect(selectedSuspect);
    }

    if (input.IsActionPressed(InputAction::PuzzleCancel)) {
        paranoia = std::min(1.0f, paranoia + 0.08f);
        statusLine = "You stop listening for a moment. The tunnel fills the gap with lies.";
        statusTimer = 1.8f;
    }
}

void TabithaTunnelPuzzle::Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const {
    const float topY = static_cast<float>(screenHeight) * 0.18f;
    const glm::vec3 titleColor(0.82f, 0.95f, 0.72f);
    const glm::vec3 warnColor(1.0f, 0.8f, 0.5f);
    const glm::vec3 calmColor(0.7f, 0.95f, 1.0f);

    textRenderer.RenderText(GetTitle(), 72.0f, topY, 0.72f, titleColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(title, 72.0f, topY + 34.0f, 0.34f, warnColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(clue, 72.0f, topY + 62.0f, 0.32f, calmColor * alpha, screenWidth, screenHeight);

    const VoiceSample& sample = samples[static_cast<std::size_t>(currentSample)];
    textRenderer.RenderText("Sample " + std::to_string(currentSample + 1) + ": " + sample.speaker, 72.0f, topY + 142.0f, 0.40f, titleColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("Line: " + sample.line, 72.0f, topY + 178.0f, 0.32f, calmColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("Breath: " + sample.breathPattern, 72.0f, topY + 210.0f, 0.32f, warnColor * alpha, screenWidth, screenHeight);

    for (int i = 0; i < 4; ++i) {
        const glm::vec3 color = (selectedSuspect == i) ? glm::vec3(1.0f, 0.96f, 0.5f) : calmColor;
        const std::string mark = identified[static_cast<std::size_t>(i)] ? " [MIMIC FOUND]" : "";
        textRenderer.RenderText(std::to_string(i + 1) + ". " + samples[static_cast<std::size_t>(i)].speaker + mark,
                                72.0f, topY + 256.0f + static_cast<float>(i) * 28.0f, 0.31f, color * alpha, screenWidth, screenHeight);
    }

    if (solved) {
        textRenderer.RenderText("THE MIMIC LOSES ITS HOLD ON THE SURVIVOR VOICES", 72.0f, static_cast<float>(screenHeight) * 0.82f, 0.42f, titleColor * alpha, screenWidth, screenHeight);
    }

    textRenderer.RenderText("[1-4] choose voice  [ENTER] accuse mimic  [ESC] steady yourself",
                           72.0f, static_cast<float>(screenHeight) - 42.0f, 0.30f, calmColor * alpha, screenWidth, screenHeight);
}

std::string TabithaTunnelPuzzle::SerializeState() const {
    std::ostringstream stream;
    stream << (solved ? 1 : 0) << ';' << currentSample << ';' << selectedSuspect << ';' << correctIdentifications << ';'
           << falseCalls << ';' << sampleTimer << ';' << paranoia;
    for (bool found : identified) {
        stream << ';' << (found ? 1 : 0);
    }
    return stream.str();
}

void TabithaTunnelPuzzle::DeserializeState(const std::string& state) {
    std::stringstream stream(state);
    std::string token;
    std::getline(stream, token, ';'); solved = (token == "1");
    std::getline(stream, token, ';'); currentSample = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';'); selectedSuspect = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';'); correctIdentifications = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';'); falseCalls = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';'); sampleTimer = token.empty() ? 0.0f : std::stof(token);
    std::getline(stream, token, ';'); paranoia = token.empty() ? 0.0f : std::stof(token);
    for (bool& found : identified) {
        if (std::getline(stream, token, ';')) {
            found = (token == "1");
        }
    }
}

void TabithaTunnelPuzzle::Reset() {
    currentSample = 0;
    selectedSuspect = 0;
    identified = {{false, false, false, false}};
    correctIdentifications = 0;
    falseCalls = 0;
    solved = false;
    sampleTimer = 0.0f;
    paranoia = 0.0f;
    statusLine.clear();
    statusTimer = 0.0f;
}
