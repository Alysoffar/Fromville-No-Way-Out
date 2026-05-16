#include "game/puzzles/logic/Jade/JadeSymbolPuzzle.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <glm/glm.hpp>

#include "engine/renderer/TextRenderer.h"
#include "engine/input/InputContext.h"
#include "game/symbols/SymbolSystem.h"

JadeSymbolPuzzle::JadeSymbolPuzzle(std::string title, std::string clue, std::string solvedMessage)
    : title(std::move(title)), clue(std::move(clue)), solvedMessage(std::move(solvedMessage)) {
}

void JadeSymbolPuzzle::Start() {
    Reset();
    statusLine = "The town lies in symbols. Translate what it means, not what it says.";
    statusTimer = 2.5f;
}

void JadeSymbolPuzzle::AdvanceFragment() {
    currentFragment++;
    if (currentFragment >= static_cast<int>(fragments.size())) {
        solved = true;
        statusLine = "The false language collapses. The true translation survives.";
        solvedMessage = "Jade finally sees through the false translation.";
        SymbolSystem::Instance().RevealSymbols(4.5f);
        return;
    }

    statusLine = "The next glyph shifts. The lie is changing shape.";
    statusTimer = 2.0f;
}

void JadeSymbolPuzzle::SubmitTranslation(int index) {
    const Fragment& fragment = fragments[static_cast<std::size_t>(currentFragment)];
    if (index == fragment.correct) {
        correctTranslations++;
        SymbolSystem::Instance().AdvanceDecoding(0.22f);
        statusLine = "That reading matches the hidden record.";
        statusTimer = 2.0f;
        hallucination = std::max(0.0f, hallucination - 0.12f);
        AdvanceFragment();
    } else {
        wrongTranslations++;
        pressure += 14.0f;
        hallucination = std::min(1.0f, hallucination + 0.22f);
        SymbolSystem::Instance().TriggerSymbolRearrangement();
        SymbolSystem::Instance().BleedSymbolsOnWall();
        statusLine = "Wrong translation. The town rewards the lie with fake lore.";
        statusTimer = 2.4f;
    }
}

void JadeSymbolPuzzle::Update(float dt, const InputContext& input) {
    timer += dt;
    statusTimer = std::max(0.0f, statusTimer - dt);
    pressure = std::max(0.0f, pressure - dt * 2.0f);
    hallucination = std::max(0.0f, hallucination - dt * 0.04f);

    if (solved) return;

    if (input.IsActionPressed(InputAction::PuzzleOption1)) selectedMeaning = 0;
    if (input.IsActionPressed(InputAction::PuzzleOption2)) selectedMeaning = 1;
    if (input.IsActionPressed(InputAction::PuzzleOption3)) selectedMeaning = 2;
    if (input.IsActionPressed(InputAction::PuzzleOption4)) selectedMeaning = 3;

    if (input.IsActionPressed(InputAction::PuzzleConfirm)) {
        SubmitTranslation(selectedMeaning);
    }

    if (input.IsActionPressed(InputAction::PuzzleCancel)) {
        pressure += 5.0f;
        SymbolSystem::Instance().TriggerSymbolRearrangement();
        statusLine = "You hesitate. The symbols begin inventing a second meaning.";
        statusTimer = 1.8f;
    }
}

void JadeSymbolPuzzle::Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const {
    const float baseY = static_cast<float>(screenHeight) * 0.18f;
    const glm::vec3 titleColor(0.8f, 0.95f, 1.0f);
    const glm::vec3 warnColor(1.0f, 0.82f, 0.42f);
    const glm::vec3 okColor(0.65f, 1.0f, 0.7f);

    textRenderer.RenderText(GetTitle(), 72.0f, baseY, 0.88f, titleColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(title, 72.0f, baseY + 34.0f, 0.46f, warnColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(clue, 72.0f, baseY + 62.0f, 0.42f, okColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("Fragment " + std::to_string(currentFragment + 1) + " / 4", 72.0f, baseY + 108.0f, 0.46f, warnColor * alpha, screenWidth, screenHeight);

    if (!solved) {
        const Fragment& fragment = fragments[static_cast<std::size_t>(currentFragment)];
        if (!fragment.visual.empty()) {
            textRenderer.RenderText("Glyph: " + fragment.glyph, 72.0f, baseY + 144.0f, 0.46f, titleColor * alpha, screenWidth, screenHeight);
            float visualY = baseY + 176.0f;
            std::stringstream vs(fragment.visual);
            std::string vline;
            while (std::getline(vs, vline)) {
                textRenderer.RenderText(vline, 72.0f, visualY, 0.64f, titleColor * alpha, screenWidth, screenHeight);
                visualY += 34.0f;
            }
            float optionsY = visualY + 18.0f;
            for (int i = 0; i < 4; ++i) {
                const glm::vec3 color = (selectedMeaning == i) ? okColor : glm::vec3(0.95f, 0.9f, 0.8f);
                textRenderer.RenderText(std::to_string(i + 1) + ". " + fragment.meanings[static_cast<std::size_t>(i)],
                                       72.0f, optionsY + static_cast<float>(i) * 28.0f, 0.40f, color * alpha, screenWidth, screenHeight);
            }
        }
    }

    if (solved) {
        textRenderer.RenderText("THE TOWN'S FALSE LANGUAGE FAILS", 72.0f, static_cast<float>(screenHeight) * 0.82f, 0.56f, okColor * alpha, screenWidth, screenHeight);
    }

    textRenderer.RenderText("[1-4] choose meaning  [ENTER] confirm  [ESC] resist", 72.0f, static_cast<float>(screenHeight) - 42.0f, 0.42f, titleColor * alpha, screenWidth, screenHeight);
}

std::string JadeSymbolPuzzle::SerializeState() const {
    std::ostringstream stream;
    stream << (solved ? 1 : 0) << ';' << currentFragment << ';' << selectedMeaning << ';' << correctTranslations << ';'
           << wrongTranslations << ';' << pressure << ';' << hallucination << ';' << timer;
    return stream.str();
}

void JadeSymbolPuzzle::DeserializeState(const std::string& state) {
    std::stringstream stream(state);
    std::string token;
    std::getline(stream, token, ';'); solved = (token == "1");
    std::getline(stream, token, ';'); currentFragment = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';'); selectedMeaning = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';'); correctTranslations = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';'); wrongTranslations = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';'); pressure = token.empty() ? 0.0f : std::stof(token);
    std::getline(stream, token, ';'); hallucination = token.empty() ? 0.0f : std::stof(token);
    std::getline(stream, token, ';'); timer = token.empty() ? 0.0f : std::stof(token);
}

void JadeSymbolPuzzle::Reset() {
    currentFragment = 0;
    selectedMeaning = 0;
    correctTranslations = 0;
    wrongTranslations = 0;
    solved = false;
    timer = 0.0f;
    pressure = 0.0f;
    hallucination = 0.0f;
    statusLine.clear();
    statusTimer = 0.0f;
}
