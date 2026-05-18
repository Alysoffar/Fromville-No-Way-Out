#include "game/puzzles/logic/Tabitha/TabithaTunnelPuzzle.h"

#include <algorithm>
#include <sstream>

#include <glm/glm.hpp>

#include "engine/renderer/TextRenderer.h"

TabithaTunnelPuzzle::TabithaTunnelPuzzle(std::string title, std::string clue, std::string solvedMessage)
    : title(std::move(title)), clue(std::move(clue)), solvedMessage(std::move(solvedMessage)) {
}

void TabithaTunnelPuzzle::Start() {
    Reset();
    statusLine = "Listen for the survivor who breathes wrong.";
    statusTimer = 2.5f;
    PlaySound("puzzle_open");
}

void TabithaTunnelPuzzle::AdvanceSample() {
    currentSample = (currentSample + 1) % static_cast<int>(samples.size());
    sampleTimer = 0.0f;
    statusLine = "Another voice crawls through the tunnel walls.";
    statusTimer = 2.0f;
    PlaySound("puzzle_tick");
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
    if (index < 0 || index >= static_cast<int>(samples.size())) {
        return;
    }

    selectedSuspect = index;
    if (identified[static_cast<std::size_t>(index)]) {
        statusLine = "You already marked that voice. Listen for another false breath.";
        statusTimer = 2.0f;
        PlaySound("puzzle_locked");
        return;
    }

    if (samples[static_cast<std::size_t>(index)].mimic) {
        identified[static_cast<std::size_t>(index)] = true;
        correctIdentifications++;
        paranoia = std::max(0.0f, paranoia - 0.15f);
        statusLine = "That breath pattern is wrong. The mimic is exposed.";
        statusTimer = 2.2f;
        PlaySound("puzzle_tick");
        TunnelMappingSystem::Instance().PlayDistortedEcho();
        if (correctIdentifications >= MimicCount()) {
            solved = true;
            statusLine = "The mimic loses the survivors' voices.";
            solvedMessage = "Tabitha isolates the false voices without accusing every survivor.";
            PlaySound("puzzle_complete");
        } else {
            AdvanceSample();
        }
    } else {
        falseCalls++;
        paranoia = std::min(1.0f, paranoia + 0.22f);
        statusLine = "Wrong target. The real survivor would never sound like that.";
        statusTimer = 2.1f;
        PlaySound("puzzle_fail");
        TunnelMappingSystem::Instance().TriggerTunnelShift();
    }
}

void TabithaTunnelPuzzle::Update(float dt, const InputContext& input) {
    sampleTimer += dt;
    statusTimer = std::max(0.0f, statusTimer - dt);
    paranoia = std::max(0.0f, paranoia - dt * 0.05f);

    if (input.IsActionPressed(InputAction::PuzzleReset)) {
        Start();
        return;
    }

    if (solved) {
        return;
    }

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
        PlaySound("puzzle_tick");
    }
}

void TabithaTunnelPuzzle::Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const {
    const float centerX = static_cast<float>(screenWidth) * 0.5f;
    const float baseY = static_cast<float>(screenHeight) * 0.28f;
    const glm::vec3 titleColor(0.82f, 0.95f, 0.72f);
    const glm::vec3 warnColor(1.0f, 0.8f, 0.5f);
    const glm::vec3 calmColor(0.7f, 0.95f, 1.0f);
    const glm::vec3 okColor(0.65f, 1.0f, 0.7f);

    // Left Panel status & paranoia
    const float leftPanelX = 72.0f;
    const float leftPanelY = static_cast<float>(screenHeight) - 340.0f;

    textRenderer.RenderText("◇ MIMIC DETECTION", leftPanelX, leftPanelY, 0.54f, warnColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("──────────────────────", leftPanelX, leftPanelY - 14.0f, 0.50f, glm::vec3(0.5f) * alpha, screenWidth, screenHeight);
    
    textRenderer.RenderText("Mimics Exposed: " + std::to_string(correctIdentifications) + "/" + std::to_string(MimicCount()), leftPanelX, leftPanelY - 40.0f, 0.52f, titleColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("Paranoia: " + std::string(static_cast<int>(paranoia * 20.0f), '!'), leftPanelX, leftPanelY - 76.0f, 0.52f, warnColor * alpha, screenWidth, screenHeight);

    if (!statusLine.empty() && statusTimer > 0.0f) {
        textRenderer.RenderText("AUDIO LOGS:", leftPanelX, leftPanelY - 140.0f, 0.50f, calmColor * alpha, screenWidth, screenHeight);
        textRenderer.RenderText(statusLine, leftPanelX, leftPanelY - 164.0f, 0.46f, warnColor * alpha, screenWidth, screenHeight);
    }

    // Centered active voice sample and multiple-choice list
    if (!solved) {
        const VoiceSample& sample = samples[static_cast<std::size_t>(currentSample)];
        std::string sampleTitle = "◇ ACTIVE AUDIO SAMPLE ◇";
        float titleX = centerX - (static_cast<float>(sampleTitle.length()) * 5.0f);
        textRenderer.RenderText(sampleTitle, titleX, baseY + 180.0f, 0.65f, titleColor * alpha, screenWidth, screenHeight);

        std::string speakerStr = "Speaker: " + sample.speaker;
        float speakerX = centerX - (static_cast<float>(speakerStr.length()) * 5.0f);
        textRenderer.RenderText(speakerStr, speakerX, baseY + 140.0f, 0.54f, titleColor * alpha, screenWidth, screenHeight);

        std::string lineStr = "\"" + sample.line + "\"";
        float lineX = centerX - (static_cast<float>(lineStr.length()) * 4.5f);
        textRenderer.RenderText(lineStr, lineX, baseY + 106.0f, 0.52f, calmColor * alpha, screenWidth, screenHeight);

        std::string breathStr = "Breath Rhythm: " + sample.breathPattern;
        float breathX = centerX - (static_cast<float>(breathStr.length()) * 4.5f);
        textRenderer.RenderText(breathStr, breathX, baseY + 76.0f, 0.52f, warnColor * alpha, screenWidth, screenHeight);

        // Options centered below
        float optionsY = baseY + 20.0f;
        for (int i = 0; i < 4; ++i) {
            const glm::vec3 color = (selectedSuspect == i) ? glm::vec3(1.0f, 0.96f, 0.5f) : calmColor;
            const std::string mark = identified[static_cast<std::size_t>(i)] ? " [MIMIC FOUND]" : "";
            std::string optionText = std::to_string(i + 1) + ". " + samples[static_cast<std::size_t>(i)].speaker + mark;
            float optX = centerX - 120.0f;
            textRenderer.RenderText(optionText, optX, optionsY - static_cast<float>(i) * 26.0f, 0.52f, color * alpha, screenWidth, screenHeight);
        }
    } else {
        textRenderer.RenderText("★ SIGNAL SECURED ★", centerX - 140.0f, baseY + 40.0f, 1.25f, okColor * alpha, screenWidth, screenHeight);
    }
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

    currentSample = std::clamp(currentSample, 0, static_cast<int>(samples.size()) - 1);
    selectedSuspect = std::clamp(selectedSuspect, 0, 3);
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
