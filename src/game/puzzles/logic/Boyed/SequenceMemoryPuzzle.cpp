#include "game/puzzles/logic/Boyed/SequenceMemoryPuzzle.h"

#include <algorithm>
#include <sstream>

#include "engine/renderer/TextRenderer.h"

SequenceMemoryPuzzle::SequenceMemoryPuzzle(std::string title, std::string clue, std::array<std::string, 4> symbols, std::array<int, 4> sequence, std::string solvedMessage)
    : title(std::move(title)), clue(std::move(clue)), symbols(std::move(symbols)), sequence(sequence), solvedMessage(std::move(solvedMessage)) {
}

void SequenceMemoryPuzzle::Start() {
    PlaySound("puzzle_open");
    revealTimer = 2.5f;
    inputIndex = 0;
}

void SequenceMemoryPuzzle::Update(float dt, const InputContext& input) {
    if (solved) {
        return;
    }

    if (revealTimer > 0.0f) {
        revealTimer = std::max(0.0f, revealTimer - dt);
        return;
    }

    for (int option = 0; option < 4; ++option) {
        if (input.IsActionPressed(static_cast<InputAction>(ToIndex(InputAction::PuzzleOption1) + option))) {
            if (sequence[inputIndex] == option) {
                ++inputIndex;
                PlaySound("puzzle_tick");
                if (inputIndex >= sequence.size()) {
                    solved = true;
                    PlaySound("puzzle_solve");
                }
            } else {
                inputIndex = 0;
                PlaySound("puzzle_fail");
            }
        }
    }
}

void SequenceMemoryPuzzle::Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const {
    const glm::vec3 titleColor(1.0f, 0.2f, 0.2f);
    const glm::vec3 textColor(0.92f, 0.92f, 0.85f);
    const glm::vec3 accentColor(0.70f, 0.90f, 1.0f);

    textRenderer.RenderText(title, 74.0f, static_cast<float>(screenHeight) - 92.0f, 0.66f, titleColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText(clue, 74.0f, static_cast<float>(screenHeight) - 130.0f, 0.42f, accentColor * alpha, screenWidth, screenHeight);
    textRenderer.RenderText("MEMORIZE THE BROKEN ORDER", 74.0f, static_cast<float>(screenHeight) - 168.0f, 0.38f, textColor * alpha, screenWidth, screenHeight);

    if (revealTimer > 0.0f) {
        std::ostringstream sequenceLine;
        sequenceLine << "SEQUENCE: ";
        for (std::size_t i = 0; i < sequence.size(); ++i) {
            sequenceLine << (i > 0 ? " - " : "") << symbols[sequence[i]];
        }
        textRenderer.RenderText(sequenceLine.str(), 74.0f, static_cast<float>(screenHeight) * 0.56f, 0.44f, accentColor * alpha, screenWidth, screenHeight);
    } else {
        std::ostringstream progress;
        progress << "REPEAT THE ORDER: " << inputIndex << "/" << sequence.size();
        textRenderer.RenderText(progress.str(), 74.0f, static_cast<float>(screenHeight) * 0.56f, 0.44f, textColor * alpha, screenWidth, screenHeight);
        for (int i = 0; i < 4; ++i) {
            std::ostringstream line;
            line << (i + 1) << ". " << symbols[i];
            textRenderer.RenderText(line.str(), 74.0f, static_cast<float>(screenHeight) * 0.50f - (static_cast<float>(i) * 26.0f), 0.40f, textColor * alpha, screenWidth, screenHeight);
        }
    }

    if (solved) {
        textRenderer.RenderText(solvedMessage, 74.0f, static_cast<float>(screenHeight) * 0.34f, 0.45f, glm::vec3(1.0f, 0.35f, 0.35f) * alpha, screenWidth, screenHeight);
    }
}

std::string SequenceMemoryPuzzle::SerializeState() const {
    return std::string(solved ? "1" : "0") + ";" + std::to_string(inputIndex) + ";" + std::to_string(revealTimer);
}

void SequenceMemoryPuzzle::DeserializeState(const std::string& state) {
    std::stringstream stream(state);
    std::string token;
    std::getline(stream, token, ';');
    solved = token == "1";
    std::getline(stream, token, ';');
    inputIndex = static_cast<std::size_t>(std::stoul(token.empty() ? "0" : token));
    std::getline(stream, token, ';');
    revealTimer = token.empty() ? 0.0f : std::stof(token);
}

void SequenceMemoryPuzzle::Reset() {
    solved = false;
    inputIndex = 0;
    revealTimer = 2.5f;
}