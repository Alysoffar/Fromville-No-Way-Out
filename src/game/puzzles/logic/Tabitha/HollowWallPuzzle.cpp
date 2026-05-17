#include "game/puzzles/logic/Tabitha/HollowWallPuzzle.h"

#include <algorithm>
#include <sstream>

#include <glm/glm.hpp>

#include "engine/renderer/TextRenderer.h"

HollowWallPuzzle::HollowWallPuzzle(std::string title, std::string clue, std::string solvedMessage)
    : title(std::move(title)), clue(std::move(clue)), solvedMessage(std::move(solvedMessage)) {
}
void HollowWallPuzzle::Start() {
    Reset();
        statusLine = "The children chant a single, strange syllable: 'ankouhi'.";
        hintLine = "Hint: the chant begins with an open vowel and rings like a name.";
        hintTimer = 6.0f;
    statusTimer = 2.6f;
    PlaySound("puzzle_open");
}

void HollowWallPuzzle::EvaluateGuess(int index) {
    if (index < 0 || index >= static_cast<int>(options.size())) return;

    selectedOption = index;
    if (index == answerIndex) {
        success = true;
        solved = true;
        statusLine = "The children suddenly speak in clear English and tell Tabitha the truth.";
        statusTimer = 3.2f;
        solvedMessage = "The children speak English: 'This place keeps our names.'";
        PlaySound("puzzle_complete");
    } else {
        guessesLeft = std::max(0, guessesLeft - 1);
        statusLine = "That isn't the right meaning. The children murmur again.";
        statusTimer = 2.2f;
        PlaySound("puzzle_fail");
        DropNextHint();
        if (guessesLeft <= 0) {
            // Out of guesses - puzzle closes with stuck message
            success = false;
            solved = true; // close the puzzle
            solvedMessage = "You are stuck in this place.";
            statusLine = "The children fall silent. You feel the walls close in.";
            statusTimer = 3.0f;
            PlaySound("puzzle_close");
        }
    }
}

void HollowWallPuzzle::DropNextHint() {
    // Reveal progressively more of the word
    if (guessesLeft == 2) {
        // after first wrong guess
        hintLine = "Hint: it begins with 'A' and ends with 'I'.";
    } else if (guessesLeft == 1) {
        // after second wrong guess
        hintLine = "Hint: the children use it when they mean 'home'.";
    } else {
        hintLine.clear();
    }
    hintTimer = 4.0f;
}

void HollowWallPuzzle::Update(float dt, const InputContext& input) {
    hintTimer = std::max(0.0f, hintTimer - dt);
    statusTimer = std::max(0.0f, statusTimer - dt);

    if (input.IsActionPressed(InputAction::PuzzleReset)) {
        Start();
        return;
    }

    if (solved) {
        return;
    }

    if (input.IsActionPressed(InputAction::PuzzleOption1)) selectedOption = 0;
    if (input.IsActionPressed(InputAction::PuzzleOption2)) selectedOption = 1;
    if (input.IsActionPressed(InputAction::PuzzleOption3)) selectedOption = 2;
    if (input.IsActionPressed(InputAction::PuzzleOption4)) selectedOption = 3;

    if (input.IsActionPressed(InputAction::PuzzleConfirm)) {
        EvaluateGuess(selectedOption);
    }

    if (input.IsActionPressed(InputAction::PuzzleCancel)) {
        statusLine = "You step back, and the children's chant fades.";
        statusTimer = 1.6f;
        PlaySound("puzzle_tick");
    }
}

void HollowWallPuzzle::Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const {
    const float topY = static_cast<float>(screenHeight) * 0.18f;
    const glm::vec3 titleColor(0.82f, 0.95f, 0.78f);
    const glm::vec3 warnColor(1.0f, 0.8f, 0.48f);
    const glm::vec3 calmColor(0.76f, 0.92f, 1.0f);

    textRenderer.RenderText(GetTitle(), 72.0f, topY, 0.72f, titleColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(title, 72.0f, topY + 34.0f, 0.34f, warnColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(clue, 72.0f, topY + 62.0f, 0.32f, calmColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("The children chant: '" + childrenWord + "'", 72.0f, topY + 96.0f, 0.36f, warnColor * alpha, screenWidth, screenHeight);

    for (int i = 0; i < 4; ++i) {
        const glm::vec3 color = (selectedOption == i) ? glm::vec3(1.0f, 0.95f, 0.5f) : calmColor;
        textRenderer.RenderText(std::to_string(i + 1) + ". " + options[static_cast<std::size_t>(i)],
                                72.0f, topY + 140.0f + static_cast<float>(i) * 30.0f, 0.36f, color * alpha, screenWidth, screenHeight);
    }

    textRenderer.RenderText("Guesses left: " + std::to_string(guessesLeft), 72.0f, static_cast<float>(screenHeight) - 160.0f, 0.36f, titleColor * alpha, screenWidth, screenHeight);

    if (!hintLine.empty() && hintTimer > 0.0f) {
        textRenderer.RenderText(hintLine, 72.0f, static_cast<float>(screenHeight) - 128.0f, 0.34f, warnColor * alpha, screenWidth, screenHeight);
    }

    if (!statusLine.empty() && statusTimer > 0.0f) {
        textRenderer.RenderText(statusLine, 72.0f, static_cast<float>(screenHeight) - 92.0f, 0.34f, warnColor * alpha, screenWidth, screenHeight);
    }

    if (solved) {
        const glm::vec3 resultColor = success ? glm::vec3(0.6f, 1.0f, 0.6f) : glm::vec3(1.0f, 0.6f, 0.6f);
        textRenderer.RenderText(solvedMessage, 72.0f, static_cast<float>(screenHeight) * 0.82f, 0.42f, resultColor * alpha, screenWidth, screenHeight);
    }

    textRenderer.RenderText("[1-4] choose meaning  [ENTER] guess  [ESC] step back  [R] restart", 72.0f, static_cast<float>(screenHeight) - 42.0f, 0.30f, titleColor * alpha, screenWidth, screenHeight);
}

std::string HollowWallPuzzle::SerializeState() const {
    std::ostringstream stream;
    stream << (solved ? 1 : 0) << ';' << (success ? 1 : 0) << ';' << selectedOption << ';' << guessesLeft << ';' << hintLine << ';' << hintTimer;
    return stream.str();
}

void HollowWallPuzzle::DeserializeState(const std::string& state) {
    std::stringstream stream(state);
    std::string token;

    std::getline(stream, token, ';'); solved = (token == "1");
    std::getline(stream, token, ';'); success = (token == "1");
    std::getline(stream, token, ';'); selectedOption = token.empty() ? 0 : std::stoi(token);
    std::getline(stream, token, ';'); guessesLeft = token.empty() ? 3 : std::stoi(token);
    std::getline(stream, hintLine, ';');
    std::getline(stream, token, ';'); hintTimer = token.empty() ? 0.0f : std::stof(token);

    selectedOption = std::clamp(selectedOption, 0, 3);
}

void HollowWallPuzzle::Reset() {
    selectedOption = 0;
    guessesLeft = 3;
    solved = false;
    success = false;
    hintLine.clear();
    hintTimer = 0.0f;
    statusLine.clear();
    statusTimer = 0.0f;
}
