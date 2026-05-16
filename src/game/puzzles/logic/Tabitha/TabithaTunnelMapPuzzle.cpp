#include "game/puzzles/logic/Tabitha/TabithaTunnelMapPuzzle.h"

#include <algorithm>
#include <sstream>

#include <glm/glm.hpp>

#include "engine/renderer/TextRenderer.h"

TabithaTunnelMapPuzzle::TabithaTunnelMapPuzzle(std::string title, std::string clue, std::string solvedMessage)
    : title(std::move(title)), clue(std::move(clue)), solvedMessage(std::move(solvedMessage)) {
}

int TabithaTunnelMapPuzzle::ExpectedValve() const {
    return reroutes % static_cast<int>(valves.size());
}

void TabithaTunnelMapPuzzle::Start() {
    Reset();
    statusLine = "The tunnel is flooding. Reroute pressure before the water takes the hall.";
    statusTimer = 2.8f;
    PlaySound("tunnel_rumble");
}

void TabithaTunnelMapPuzzle::RerouteValve(int index) {
    if (index < 0 || index >= static_cast<int>(valves.size())) {
        return;
    }

    selectedValve = index;
    if (index == ExpectedValve()) {
        reroutes++;
        pressureLevel = std::max(0.0f, pressureLevel - 6.0f);
        waterLevel = std::max(0.0f, waterLevel - 14.0f);
        TunnelMappingSystem::Instance().FloatThroughWater(0.35f);
        statusLine = valves[static_cast<std::size_t>(index)] + " is holding. The flood recedes.";
        statusTimer = 2.0f;
        PlaySound("puzzle_tick");
        if (reroutes >= 5) {
            solved = true;
            statusLine = "The routes hold and the flooded tunnel can breathe again.";
            solvedMessage = "Tabitha survives the flood by rerouting pressure in time.";
            PlaySound("puzzle_complete");
            TunnelMappingSystem::Instance().ReachCentralChamber();
        }
    } else {
        waterLevel = std::min(100.0f, waterLevel + 12.0f);
        pressureLevel = std::min(100.0f, pressureLevel + 9.0f);
        statusLine = "Wrong valve. The flood surges into the wrong branch.";
        statusTimer = 2.2f;
        PlaySound("puzzle_fail");
        TunnelMappingSystem::Instance().TriggerTunnelShift();
    }
}

void TabithaTunnelMapPuzzle::Update(float dt, const InputContext& input) {
    pulse += dt;
    statusTimer = std::max(0.0f, statusTimer - dt);
    waterLevel = std::min(100.0f, waterLevel + dt * 8.5f);
    pressureLevel = std::min(100.0f, pressureLevel + dt * 2.5f);

    if (input.IsActionPressed(InputAction::PuzzleReset)) {
        Start();
        return;
    }

    if (solved) {
        return;
    }

    if (waterLevel >= 100.0f) {
        solved = false;
        statusLine = "The tunnel drowns before the pressure can be rerouted.";
        statusTimer = 2.5f;
        PlaySound("puzzle_fail");
        return;
    }

    if (input.IsActionPressed(InputAction::PuzzleOption1)) selectedValve = 0;
    if (input.IsActionPressed(InputAction::PuzzleOption2)) selectedValve = 1;
    if (input.IsActionPressed(InputAction::PuzzleOption3)) selectedValve = 2;
    if (input.IsActionPressed(InputAction::PuzzleOption4)) selectedValve = 3;

    if (input.IsActionPressed(InputAction::PuzzleConfirm)) {
        RerouteValve(selectedValve);
    }

    if (input.IsActionPressed(InputAction::PuzzleCancel)) {
        pressureLevel = std::min(100.0f, pressureLevel + 5.0f);
        statusLine = "You hesitate and hear water move behind the wall.";
        statusTimer = 1.8f;
        TunnelMappingSystem::Instance().PlayDistortedEcho();
        PlaySound("puzzle_tick");
    }
}

void TabithaTunnelMapPuzzle::Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const {
    const float topY = static_cast<float>(screenHeight) * 0.18f;
    const glm::vec3 titleColor(0.9f, 0.74f, 1.0f);
    const glm::vec3 warnColor(1.0f, 0.79f, 0.47f);
    const glm::vec3 calmColor(0.72f, 0.95f, 1.0f);

    textRenderer.RenderText(GetTitle(), 72.0f, topY, 0.68f, titleColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(title, 72.0f, topY + 34.0f, 0.34f, warnColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(clue, 72.0f, topY + 62.0f, 0.32f, calmColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("Reroute valves in the safe pressure order. The next safe valve is shown below.", 72.0f, topY + 96.0f, 0.32f, warnColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("Next safe valve: " + valves[static_cast<std::size_t>(ExpectedValve())], 72.0f, topY + 122.0f, 0.36f, glm::vec3(1.0f, 0.95f, 0.5f) * alpha, screenWidth, screenHeight);

    for (int i = 0; i < 4; ++i) {
        const glm::vec3 color = (selectedValve == i) ? glm::vec3(1.0f, 0.95f, 0.5f)
                               : (ExpectedValve() == i ? glm::vec3(0.55f, 1.0f, 0.65f) : calmColor);
        textRenderer.RenderText(std::to_string(i + 1) + ". " + valves[static_cast<std::size_t>(i)],
                                72.0f, topY + 164.0f + static_cast<float>(i) * 30.0f, 0.31f, color * alpha, screenWidth, screenHeight);
    }

    textRenderer.RenderText("Water Level: " + std::to_string(static_cast<int>(waterLevel)) + "%", 72.0f, static_cast<float>(screenHeight) - 168.0f, 0.36f, warnColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("Pressure: " + std::to_string(static_cast<int>(pressureLevel)) + "%", 72.0f, static_cast<float>(screenHeight) - 136.0f, 0.36f, calmColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("Successful reroutes: " + std::to_string(reroutes) + "/5", 72.0f, static_cast<float>(screenHeight) - 104.0f, 0.34f, titleColor * alpha, screenWidth, screenHeight);

    if (!statusLine.empty() && statusTimer > 0.0f) {
        textRenderer.RenderText(statusLine, 72.0f, static_cast<float>(screenHeight) - 70.0f, 0.34f, (solved ? calmColor : warnColor) * alpha, screenWidth, screenHeight);
    }

    if (solved) {
        textRenderer.RenderText("THE FLOODED TUNNEL OPENS AGAIN", 72.0f, static_cast<float>(screenHeight) * 0.82f, 0.42f, calmColor * alpha, screenWidth, screenHeight);
    }

    textRenderer.RenderText("[1-4] select valve  [ENTER] reroute  [ESC] resist  [R] restart", 72.0f, static_cast<float>(screenHeight) - 42.0f, 0.30f, titleColor * alpha, screenWidth, screenHeight);
}

std::string TabithaTunnelMapPuzzle::SerializeState() const {
    std::ostringstream stream;
    stream << (solved ? 1 : 0) << ';' << selectedValve << ';' << reroutes << ';' << waterLevel << ';' << pressureLevel << ';' << pulse;
    return stream.str();
}

void TabithaTunnelMapPuzzle::DeserializeState(const std::string& state) {
    std::stringstream stream(state);
    std::string token;

    std::getline(stream, token, ';'); solved = (token == "1");
    std::getline(stream, token, ';'); selectedValve = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';'); reroutes = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';'); waterLevel = token.empty() ? 18.0f : std::stof(token);
    std::getline(stream, token, ';'); pressureLevel = token.empty() ? 20.0f : std::stof(token);
    std::getline(stream, token, ';'); pulse = token.empty() ? 0.0f : std::stof(token);
}

void TabithaTunnelMapPuzzle::Reset() {
    selectedValve = 0;
    reroutes = 0;
    solved = false;
    waterLevel = 18.0f;
    pressureLevel = 20.0f;
    pulse = 0.0f;
    statusLine.clear();
    statusTimer = 0.0f;
}
