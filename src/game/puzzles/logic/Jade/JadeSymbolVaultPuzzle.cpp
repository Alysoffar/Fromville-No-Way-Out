#include "game/puzzles/logic/Jade/JadeSymbolVaultPuzzle.h"

#include <algorithm>
#include <cmath>
#include <sstream>

#include <glm/glm.hpp>

#include "engine/renderer/TextRenderer.h"

namespace {
const char* SymbolName(SymbolType type) {
    switch (type) {
        case SymbolType::Knowledge: return "Knowledge";
        case SymbolType::Power: return "Power";
        case SymbolType::Truth: return "Truth";
        case SymbolType::Crossing: return "Crossing";
        case SymbolType::Sacrifice: return "Sacrifice";
        case SymbolType::Awakening: return "Awakening";
        case SymbolType::Void: return "Void";
        case SymbolType::Witness: return "Witness";
    }
    return "Unknown";
}

std::string InfectionBar(float value) {
    const int filled = std::clamp(static_cast<int>(std::round(value * 20.0f)), 0, 20);
    return "[" + std::string(static_cast<std::size_t>(filled), '#') + std::string(static_cast<std::size_t>(20 - filled), '-') + "]";
}
}

JadeSymbolVaultPuzzle::JadeSymbolVaultPuzzle(std::string title, std::string clue, std::string solvedMessage)
    : title(std::move(title)), clue(std::move(clue)), solvedMessage(std::move(solvedMessage)) {
}

void JadeSymbolVaultPuzzle::Start() {
    Reset();
    statusLine = "The vault is alive. Cleanse the glyphs before they infect the room.";
    statusTimer = 2.8f;
    PlaySound("puzzle_open");
}

void JadeSymbolVaultPuzzle::SpreadInfection() {
    for (std::size_t i = 0; i < infection.size(); ++i) {
        if (stabilized[i]) {
            continue;
        }
        infection[i] = std::min(1.0f, infection[i] + 0.10f);
        const std::size_t left = (i + infection.size() - 1) % infection.size();
        const std::size_t right = (i + 1) % infection.size();
        if (!stabilized[left]) infection[left] = std::min(1.0f, infection[left] + 0.04f);
        if (!stabilized[right]) infection[right] = std::min(1.0f, infection[right] + 0.04f);
    }
    roomDistortion = std::min(1.0f, roomDistortion + 0.10f);
    SymbolSystem::Instance().TriggerSymbolRearrangement();
    SymbolSystem::Instance().BleedSymbolsOnWall();
}

void JadeSymbolVaultPuzzle::StabilizeNode(int index) {
    if (index < 0 || index >= static_cast<int>(infection.size())) {
        return;
    }

    activeNode = index;
    if (stabilized[static_cast<std::size_t>(index)]) {
        statusLine = "That glyph is already stabilized.";
        statusTimer = 1.3f;
        PlaySound("puzzle_locked");
        return;
    }

    const float target = 0.20f + 0.15f * static_cast<float>(index);
    const float delta = std::abs(infection[static_cast<std::size_t>(index)] - target);
    if (delta <= 0.18f) {
        stabilized[static_cast<std::size_t>(index)] = true;
        cleansedCount++;
        infection[static_cast<std::size_t>(index)] = 0.0f;
        roomDistortion = std::max(0.0f, roomDistortion - 0.12f);
        statusLine = std::string("The ") + SymbolName(vaultOrder[static_cast<std::size_t>(index)]) + " glyph quiets down.";
        statusTimer = 2.0f;
        PlaySound("puzzle_tick");
        SymbolSystem::Instance().AdvanceDecoding(0.15f);
        if (cleansedCount >= 4) {
            solved = true;
            statusLine = "The living symbols are contained. The vault stops breathing.";
            solvedMessage = "Jade seals the living infection before it can spread further.";
            PlaySound("puzzle_complete");
        }
    } else {
        infection[static_cast<std::size_t>(index)] = std::min(1.0f, infection[static_cast<std::size_t>(index)] + 0.18f);
        statusLine = "Wrong stabilization. The glyph mutates and spreads.";
        statusTimer = 2.0f;
        roomDistortion = std::min(1.0f, roomDistortion + 0.18f);
        PlaySound("puzzle_fail");
        SpreadInfection();
    }
}

void JadeSymbolVaultPuzzle::Update(float dt, const InputContext& input) {
    timer += dt;
    spreadTimer += dt;
    statusTimer = std::max(0.0f, statusTimer - dt);

    if (input.IsActionPressed(InputAction::PuzzleReset)) {
        Start();
        return;
    }

    if (solved) {
        return;
    }

    if (spreadTimer >= 2.4f) {
        spreadTimer = 0.0f;
        SpreadInfection();
    }

    if (input.IsActionPressed(InputAction::PuzzleOption1)) StabilizeNode(0);
    if (input.IsActionPressed(InputAction::PuzzleOption2)) StabilizeNode(1);
    if (input.IsActionPressed(InputAction::PuzzleOption3)) StabilizeNode(2);
    if (input.IsActionPressed(InputAction::PuzzleOption4)) StabilizeNode(3);

    if (input.IsActionPressed(InputAction::PuzzleConfirm)) {
        StabilizeNode(activeNode);
    }

    if (input.IsActionPressed(InputAction::PuzzleCancel)) {
        statusLine = "The room whispers louder. It is not ready yet.";
        statusTimer = 1.5f;
        roomDistortion = std::min(1.0f, roomDistortion + 0.08f);
        SymbolSystem::Instance().TriggerSymbolRearrangement();
        PlaySound("puzzle_tick");
    }
}

void JadeSymbolVaultPuzzle::Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const {
    const float baseY = static_cast<float>(screenHeight) * 0.20f;
    const glm::vec3 titleColor(0.72f, 0.96f, 0.86f);
    const glm::vec3 warnColor(1.0f, 0.79f, 0.47f);
    const glm::vec3 safeColor(0.68f, 1.0f, 0.68f);

    textRenderer.RenderText(GetTitle(), 72.0f, baseY, 0.68f, titleColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(title, 72.0f, baseY + 34.0f, 0.34f, warnColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(clue, 72.0f, baseY + 62.0f, 0.32f, safeColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("Choose the glyph to stabilize before it infects the others.", 72.0f, baseY + 96.0f, 0.32f, warnColor * alpha, screenWidth, screenHeight);

    for (int i = 0; i < 4; ++i) {
        const bool isSelected = (activeNode == i);
        const glm::vec3 color = isSelected ? safeColor : glm::vec3(0.95f, 0.9f, 0.8f);
        std::ostringstream line;
        line << (i + 1) << ". " << SymbolName(vaultOrder[static_cast<std::size_t>(i)]) << "  " << InfectionBar(infection[static_cast<std::size_t>(i)]);
        if (stabilized[static_cast<std::size_t>(i)]) {
            line << " [SEALED]";
        }
        textRenderer.RenderText(line.str(), 72.0f, baseY + 140.0f + static_cast<float>(i) * 30.0f, 0.30f, color * alpha, screenWidth, screenHeight);
    }

    std::ostringstream sealed;
    sealed << "Cleansed Glyphs: " << cleansedCount << "/4";
    textRenderer.RenderText(sealed.str(), 72.0f, static_cast<float>(screenHeight) - 162.0f, 0.36f, warnColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("Room Distortion: " + InfectionBar(roomDistortion), 72.0f, static_cast<float>(screenHeight) - 130.0f, 0.34f, warnColor * alpha, screenWidth, screenHeight);

    if (!statusLine.empty() && statusTimer > 0.0f) {
        textRenderer.RenderText(statusLine, 72.0f, static_cast<float>(screenHeight) - 94.0f, 0.34f, (solved ? safeColor : warnColor) * alpha, screenWidth, screenHeight);
    }

    if (solved) {
        textRenderer.RenderText("THE VAULT STOPS BREATHING", 72.0f, static_cast<float>(screenHeight) * 0.82f, 0.44f, safeColor * alpha, screenWidth, screenHeight);
    }

    textRenderer.RenderText("[1-4] choose glyph  [ENTER] stabilize  [L] resist  [R] restart", 72.0f, static_cast<float>(screenHeight) - 42.0f, 0.30f, titleColor * alpha, screenWidth, screenHeight);
}

std::string JadeSymbolVaultPuzzle::SerializeState() const {
    std::ostringstream stream;
    stream << (solved ? 1 : 0) << ';' << activeNode << ';' << cleansedCount << ';' << timer << ';' << spreadTimer << ';' << roomDistortion << ';';
    for (std::size_t i = 0; i < infection.size(); ++i) {
        if (i > 0) stream << ',';
        stream << infection[i] << ':' << (stabilized[i] ? 1 : 0);
    }
    return stream.str();
}

void JadeSymbolVaultPuzzle::DeserializeState(const std::string& state) {
    std::stringstream stream(state);
    std::string token;

    std::getline(stream, token, ';'); solved = (token == "1");
    std::getline(stream, token, ';'); activeNode = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';'); cleansedCount = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';'); timer = token.empty() ? 0.0f : std::stof(token);
    std::getline(stream, token, ';'); spreadTimer = token.empty() ? 0.0f : std::stof(token);
    std::getline(stream, token, ';'); roomDistortion = token.empty() ? 0.0f : std::stof(token);
    std::getline(stream, token, ';');

    for (std::size_t i = 0; i < infection.size(); ++i) {
        infection[i] = 0.0f;
        stabilized[i] = false;
    }

    if (!token.empty()) {
        std::stringstream itemStream(token);
        std::string item;
        std::size_t index = 0;
        while (std::getline(itemStream, item, ',') && index < infection.size()) {
            const std::size_t sep = item.find(':');
            infection[index] = (sep == std::string::npos) ? std::stof(item) : std::stof(item.substr(0, sep));
            stabilized[index] = (sep != std::string::npos) ? (item.substr(sep + 1) == "1") : false;
            ++index;
        }
    }
}

void JadeSymbolVaultPuzzle::Reset() {
    activeNode = 0;
    cleansedCount = 0;
    solved = false;
    timer = 0.0f;
    spreadTimer = 0.0f;
    roomDistortion = 0.0f;
    statusLine.clear();
    statusTimer = 0.0f;
    infection = {{0.15f, 0.08f, 0.12f, 0.05f}};
    stabilized = {{false, false, false, false}};
}
