#include "game/puzzles/PuzzleManager.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <nlohmann/json.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

#include "engine/core/InputManager.h"
#include "engine/renderer/Mesh.h"
#include "engine/renderer/Shader.h"
#include "engine/renderer/TextRenderer.h"
#include "game/puzzles/CipherDecodePuzzle.h"
#include "game/puzzles/ConnectTheCluesPuzzle.h"
#include "game/puzzles/SequenceMemoryPuzzle.h"
#include "game/puzzles/SymbolMatchPuzzle.h"
#include "game/puzzles/WordScramblePuzzle.h"

namespace {
std::string ToUpper(std::string value) {
    for (char& ch : value) {
        if (ch >= 'a' && ch <= 'z') {
            ch = static_cast<char>(ch - 'a' + 'A');
        }
    }
    return value;
}

std::array<std::string, 4> BuildOptionSet(const std::string& answer) {
    std::array<std::string, 4> options = {
        ToUpper(answer),
        "THE RIVER KNOWS",
        "DON'T TRUST THE LIGHTS",
        "THE TOWN IS LYING"
    };
    return options;
}

std::array<std::string, 4> BuildSymbolSet(const std::string& answer) {
    return {
        "▲ " + answer,
        "◆ MEMORY",
        "✦ STATIC",
        "◼ WITNESS"
    };
}

std::vector<std::string> SplitClues(const std::vector<std::string>& clues) {
    if (clues.empty()) {
        return {"Follow the trace", "Find the note", "Link the witness", "Open the way"};
    }
    return clues;
}
}

PuzzleManager::PuzzleManager()
    : overlayShader("PuzzleOverlay") {
}

PuzzleManager::~PuzzleManager() = default;

void PuzzleManager::Initialize() {
    if (overlayReady) {
        return;
    }

    overlayShader.Load("assets/shaders/puzzle_overlay.vert", "assets/shaders/puzzle_overlay.frag");

    const std::vector<MeshVertex> vertices = {
        {{-1.0f, -1.0f, 0.0f}},
        {{ 1.0f, -1.0f, 0.0f}},
        {{ 1.0f,  1.0f, 0.0f}},
        {{-1.0f,  1.0f, 0.0f}}
    };
    const std::vector<unsigned int> indices = {0, 1, 2, 2, 3, 0};
    overlayMesh.Create(vertices, indices);
    overlayReady = overlayMesh.IsValid();
}

void PuzzleManager::SetCompletionCallback(CompletionCallback callback) {
    completionCallback = std::move(callback);
}

void PuzzleManager::SetSoundHook(PuzzleBase::SoundHook hook) {
    soundHook = std::move(hook);
}

std::string PuzzleManager::MakePuzzleKey(CharacterType questCharacter, int objectiveIndex) {
    std::ostringstream stream;
    stream << static_cast<int>(questCharacter) << ':' << objectiveIndex;
    return stream.str();
}

bool PuzzleManager::IsSolved(const std::string& puzzleKey) const {
    return solvedPuzzleKeys.find(puzzleKey) != solvedPuzzleKeys.end();
}

PuzzleType PuzzleManager::GetPuzzleTypeForObjective(CharacterType, int objectiveIndex) {
    switch (objectiveIndex % 5) {
        case 0: return PuzzleType::WordScramble;
        case 1: return PuzzleType::SymbolMatch;
        case 2: return PuzzleType::SequenceMemory;
        case 3: return PuzzleType::CipherDecode;
        default: return PuzzleType::ConnectTheClues;
    }
}

std::string PuzzleManager::ShiftCipher(const std::string& value, int offset) {
    std::string encoded = value;
    for (char& ch : encoded) {
        if (ch >= 'A' && ch <= 'Z') {
            ch = static_cast<char>('A' + ((ch - 'A' + offset) % 26));
        }
    }
    return encoded;
}

std::array<std::string, 4> PuzzleManager::MakeCreepyOptions(const std::string& answer) {
    return {
        answer,
        "THE LIGHTS ARE WATCHING",
        "THE BASEMENT REMEMBERS",
        "DO NOT OPEN THE DOOR"
    };
}

std::array<int, 4> PuzzleManager::MakeSequenceFromKey(const std::string& key) {
    std::array<int, 4> sequence = {0, 1, 2, 3};
    const std::size_t seed = std::hash<std::string>{}(key);
    for (std::size_t i = 0; i < sequence.size(); ++i) {
        const std::size_t swapIndex = (seed + i * 17) % sequence.size();
        std::swap(sequence[i], sequence[swapIndex]);
    }
    return sequence;
}

std::unique_ptr<PuzzleBase> PuzzleManager::CreatePuzzle(PuzzleType puzzleType,
                                                        const std::string& questTitle,
                                                        const QuestObjective& objective,
                                                        CharacterType questCharacter,
                                                        int objectiveIndex) const {
    const std::string key = MakePuzzleKey(questCharacter, objectiveIndex);
    const std::string clue = objective.environmentalClues.empty() ? objective.description : objective.environmentalClues.front();

    switch (puzzleType) {
        case PuzzleType::WordScramble: {
            const std::array<std::string, 4> options = BuildOptionSet(ToUpper(objective.description));
            return std::make_unique<WordScramblePuzzle>(questTitle, clue, ToUpper(objective.description), options,
                                                        "SOLVED: the evidence fits together.");
        }
        case PuzzleType::SymbolMatch: {
            const std::array<std::string, 4> symbols = BuildSymbolSet(objective.requiredCharacter.empty() ? "RESIDENT" : objective.requiredCharacter);
            return std::make_unique<SymbolMatchPuzzle>(questTitle, clue, symbols, static_cast<int>(objectiveIndex % 4),
                                                       "SOLVED: the sigil has been matched.");
        }
        case PuzzleType::SequenceMemory: {
            const std::array<int, 4> sequence = MakeSequenceFromKey(key);
                return std::make_unique<SequenceMemoryPuzzle>(questTitle, clue,
                                                              std::array<std::string, 4>{"RUST", "HUSH", "STATIC", "ASH"},
                                                              sequence,
                                                              "SOLVED: the sequence has been restored.");
        }
        case PuzzleType::CipherDecode: {
            const std::string encoded = ShiftCipher(ToUpper(objective.description), 3);
            const std::array<std::string, 4> options = MakeCreepyOptions(ToUpper(objective.description));
            return std::make_unique<CipherDecodePuzzle>(questTitle, clue, encoded, options, 0,
                                                        "SOLVED: the hidden text has been decoded.");
        }
        case PuzzleType::ConnectTheClues: {
            std::vector<std::string> clues = SplitClues(objective.environmentalClues);
            while (clues.size() < 4) {
                clues.push_back("A missing detail waits in the dark.");
            }
                return std::make_unique<ConnectTheCluesPuzzle>(questTitle, clue, clues, std::array<int, 4>{0, 1, 2, 3},
                                                           "SOLVED: the board reveals the next lead.");
        }
    }

    return nullptr;
}

bool PuzzleManager::StartPuzzle(CharacterType questCharacter, int objectiveIndex, const QuestObjective& objective, const std::string& questTitle) {
    const std::string puzzleKey = MakePuzzleKey(questCharacter, objectiveIndex);
    if (IsSolved(puzzleKey)) {
        solvedMessage = "This clue is already solved.";
        solvedMessageTimer = solvedMessageHold;
        if (soundHook) {
            soundHook("puzzle_locked");
        }
        return false;
    }

    activeContext.questCharacter = questCharacter;
    activeContext.objectiveIndex = objectiveIndex;
    activeContext.questTitle = questTitle;
    activeContext.puzzleKey = puzzleKey;

    const PuzzleType puzzleType = GetPuzzleTypeForObjective(questCharacter, objectiveIndex);
    activePuzzle = CreatePuzzle(puzzleType, questTitle, objective, questCharacter, objectiveIndex);
    if (!activePuzzle) {
        return false;
    }

    activePuzzle->SetSoundHook([this](const std::string& cue) {
        if (soundHook) {
            soundHook(cue);
        } else {
            std::cout << "[PuzzleSound] " << cue << "\n";
        }
    });
    activePuzzle->Start();
    overlayAlpha = 0.0f;
    solvedMessage.clear();
    solvedMessageTimer = 0.0f;
    return true;
}

void PuzzleManager::CompleteActivePuzzle() {
    if (!activePuzzle) {
        return;
    }

    solvedPuzzleKeys.insert(activeContext.puzzleKey);
    solvedMessage = activePuzzle->GetSolvedMessage();
    solvedMessageTimer = solvedMessageHold;

    if (completionCallback) {
        completionCallback(activeContext.questCharacter, activeContext.objectiveIndex);
    }

    if (soundHook) {
        soundHook("puzzle_complete");
    }
    activePuzzle.reset();
    overlayAlpha = 0.0f;
}

void PuzzleManager::Update(float dt, const InputManager& input) {
    if (activePuzzle) {
        overlayAlpha = std::min(1.0f, overlayAlpha + dt * 3.5f);
        activePuzzle->Update(dt, input);
        if (activePuzzle->IsSolved()) {
            CompleteActivePuzzle();
        }
    } else {
        overlayAlpha = std::max(0.0f, overlayAlpha - dt * 2.5f);
    }

    if (solvedMessageTimer > 0.0f) {
        solvedMessageTimer = std::max(0.0f, solvedMessageTimer - dt);
    }

    // Toggle hint display with H
    if (input.IsKeyPressed(GLFW_KEY_H)) {
        showHint = !showHint;
        if (showHint && activePuzzle) {
            hintText = activePuzzle->GetClueText();
        } else {
            hintText.clear();
        }
    }

    // Allow closing puzzle with ESC
    if (input.IsKeyPressed(GLFW_KEY_ESCAPE)) {
        CancelActivePuzzle();
    }
}

void PuzzleManager::Render(TextRenderer& textRenderer, int screenWidth, int screenHeight) const {
    if (overlayReady && overlayAlpha > 0.0f) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);

        overlayShader.Bind();
        overlayShader.SetFloat("uAlpha", 0.82f * overlayAlpha);
        overlayShader.SetVec3("uTint", glm::vec3(0.02f, 0.02f, 0.03f));
        overlayMesh.Draw();
        overlayShader.Unbind();

        glEnable(GL_DEPTH_TEST);
    }

    if (activePuzzle) {
        // Draw a centered modal UI box header and controls with distinct shapes
        const float centerX = static_cast<float>(screenWidth) * 0.5f;
        const float topY = static_cast<float>(screenHeight) - 100.0f;
        const std::string title = activePuzzle->GetTitle();
        const std::string objective = activePuzzle->GetClueText();

        // ◆ PUZZLE HEADER (circle marker)
        textRenderer.RenderText("◆ PUZZLE ◆", centerX - 200.0f, topY + 40.0f, 0.8f, glm::vec3(1.0f, 0.7f, 0.3f) * overlayAlpha, screenWidth, screenHeight);
        
        // Puzzle title (in center, spaced down)
        textRenderer.RenderText(title, centerX - 280.0f, topY, 1.3f, glm::vec3(1.0f, 0.95f, 0.8f) * overlayAlpha, screenWidth, screenHeight);
        
        // Blank line for spacing
        textRenderer.RenderText("", centerX, topY - 30.0f, 0.5f, glm::vec3(1.0f, 1.0f, 1.0f), screenWidth, screenHeight);
        
        // Objective / instructions (clearly labeled)
        textRenderer.RenderText("INSTRUCTIONS:", centerX - 280.0f, topY - 50.0f, 0.7f, glm::vec3(0.9f, 0.9f, 0.7f) * overlayAlpha, screenWidth, screenHeight);
        textRenderer.RenderText(objective, centerX - 280.0f, topY - 85.0f, 0.65f, glm::vec3(0.85f, 0.85f, 0.85f) * overlayAlpha, screenWidth, screenHeight);
        
        // Blank line for spacing
        textRenderer.RenderText("", centerX, topY - 130.0f, 0.5f, glm::vec3(1.0f, 1.0f, 1.0f), screenWidth, screenHeight);

        // Controls line (diamond marker for distinction)
        textRenderer.RenderText("◇ CONTROLS: [H] = Hint  |  [ESC] = Close Puzzle", centerX - 280.0f, topY - 145.0f, 0.55f, glm::vec3(0.8f, 0.8f, 0.8f) * overlayAlpha, screenWidth, screenHeight);

        // Draw the puzzle itself with good spacing below
        activePuzzle->Render(textRenderer, screenWidth, screenHeight, overlayAlpha);

        if (overlayAlpha > 0.0f) {
            const std::string solvedBanner = "★ PUZZLE SOLVED ★";
            if (solvedMessageTimer > 0.0f) {
                textRenderer.RenderText(solvedBanner, centerX - 220.0f, static_cast<float>(screenHeight) * 0.45f, 1.8f, glm::vec3(0.3f, 1.0f, 0.3f), screenWidth, screenHeight);
            }
        }
    }

    if (!solvedMessage.empty() && solvedMessageTimer > 0.0f) {
        textRenderer.RenderText(solvedMessage, static_cast<float>(screenWidth) * 0.08f, static_cast<float>(screenHeight) * 0.22f, 0.50f, glm::vec3(1.0f, 0.25f, 0.25f) * std::min(1.0f, solvedMessageTimer / solvedMessageHold), screenWidth, screenHeight);
    }
}

void PuzzleManager::CancelActivePuzzle() {
    activePuzzle.reset();
    overlayAlpha = 0.0f;
}

void PuzzleManager::ResetSolvedState() {
    solvedPuzzleKeys.clear();
}

std::string PuzzleManager::SerializeState() const {
    nlohmann::json json;
    json["solved"] = std::vector<std::string>(solvedPuzzleKeys.begin(), solvedPuzzleKeys.end());
    return json.dump();
}

void PuzzleManager::DeserializeState(const std::string& state) {
    solvedPuzzleKeys.clear();
    if (state.empty()) {
        return;
    }

    try {
        const nlohmann::json json = nlohmann::json::parse(state);
        if (json.contains("solved")) {
            for (const auto& entry : json["solved"]) {
                solvedPuzzleKeys.insert(entry.get<std::string>());
            }
        }
    } catch (...) {
        solvedPuzzleKeys.clear();
    }
}

std::string PuzzleManager::GetOverlayTitle() const {
    return activePuzzle ? activePuzzle->GetTitle() : "";
}

std::string PuzzleManager::GetOverlayClue() const {
    return activePuzzle ? activePuzzle->GetClueText() : "";
}

std::string PuzzleManager::GetSolvedMessage() const {
    return solvedMessageTimer > 0.0f ? solvedMessage : "";
}