#include "game/puzzles/core/PuzzleManager.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <nlohmann/json.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

#include "engine/renderer/Mesh.h"
#include "engine/renderer/Shader.h"
#include "engine/renderer/TextRenderer.h"

#include "game/puzzles/logic/Boyed/CipherDecodePuzzle.h"
#include "game/puzzles/logic/Jade/JadeSymbolPuzzle.h"
#include "game/puzzles/logic/Tabitha/TabithaTunnelPuzzle.h"
#include "game/puzzles/logic/Victor/VictorMemoryTimelinePuzzle.h"

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

void PuzzleManager::SetConsequenceHook(ConsequenceCallback hook) {
    consequenceHook = std::move(hook);
}

std::string PuzzleManager::MakePuzzleKey(CharacterType questCharacter, int objectiveIndex) {
    std::ostringstream stream;
    stream << static_cast<int>(questCharacter) << ':' << objectiveIndex;
    return stream.str();
}

bool PuzzleManager::IsSolved(const std::string& puzzleKey) const {
    return solvedPuzzleKeys.find(puzzleKey) != solvedPuzzleKeys.end();
}

PuzzleType PuzzleManager::GetPuzzleTypeForObjective(CharacterType questCharacter, int objectiveIndex, int subObjectiveIndex) {
    if (questCharacter == CharacterType::Boyd) {
        if (objectiveIndex == 0) return PuzzleType::MaraTrustDialogue;
        if (objectiveIndex == 1 && subObjectiveIndex == 1) return PuzzleType::LedgerRotation;
        if (objectiveIndex == 2) return PuzzleType::EvidenceBoard;
        if (objectiveIndex == 3) return PuzzleType::CultDebate;
        if (objectiveIndex == 4) return PuzzleType::RitualInference;
    }

    if (questCharacter == CharacterType::Jade) {
        if (objectiveIndex == 0) return PuzzleType::JadeSymbolPuzzle;
        if (objectiveIndex == 1) return PuzzleType::JadeSymbolVaultPuzzle;
        if (objectiveIndex == 4) return PuzzleType::JadeDebatePuzzle;
        switch ((objectiveIndex - 2) % 5) {
            case 0: return PuzzleType::WordScramble;
            case 1: return PuzzleType::SymbolMatch;
            case 2: return PuzzleType::SequenceMemory;
            case 3: return PuzzleType::CipherDecode;
            default: return PuzzleType::ConnectTheClues;
        }
    }

    if (questCharacter == CharacterType::Tabitha) {
        if (objectiveIndex == 0) return PuzzleType::TabithaTunnelPuzzle;
        if (objectiveIndex == 1) return PuzzleType::TabithaTunnelMapPuzzle;
        if (objectiveIndex == 2) return PuzzleType::TabithaWhisperRoutePuzzle;
        if (objectiveIndex == 3) return PuzzleType::TabithaEchoDialoguePuzzle;
        return PuzzleType::HollowWallPuzzle;
    }

    if (questCharacter == CharacterType::Victor) {
        if (objectiveIndex == 0) return PuzzleType::GhostWitnessPuzzle;
        if (objectiveIndex == 1) return PuzzleType::VictorEmptyChairPuzzle;
        if (objectiveIndex == 2) return PuzzleType::VictorMirrorCollapsePuzzle;
        if (objectiveIndex == 3) return PuzzleType::VictorSilentProcessionPuzzle;
        return PuzzleType::VictorMemoryPuzzle;
    }

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
                                                        int objectiveIndex,
                                                        int subObjectiveIndex) const {
    switch (puzzleType) {
        case PuzzleType::CipherDecode:
            return std::make_unique<CipherDecodePuzzle>(
                questTitle, objective.description, "Z-S-S-Y-R-H-S-N", 
                BuildOptionSet("ALYSSE"), 0, "The town ledger reveals a name from 1985.");
        
        case PuzzleType::JadeSymbolPuzzle:
            return std::make_unique<JadeSymbolPuzzle>(
                questTitle, objective.description, "The symbols reveal their hidden intent.");
                
        case PuzzleType::TabithaTunnelPuzzle:
            return std::make_unique<TabithaTunnelPuzzle>(
                questTitle, objective.description, "The mimic is forced to retreat.");
                
        case PuzzleType::VictorMemoryPuzzle:
            return std::make_unique<VictorMemoryTimelinePuzzle>(
                questTitle, objective.description, "The timeline is restored.");
                
        default:
            // Placeholder for other types
            return nullptr;
    }
}

bool PuzzleManager::StartPuzzle(CharacterType questCharacter, int objectiveIndex, const QuestObjective& objective, const std::string& questTitle) {
    return StartPuzzle(questCharacter, objectiveIndex, -1, objective, questTitle);
}

bool PuzzleManager::StartPuzzle(CharacterType questCharacter, int objectiveIndex, int subObjectiveIndex, const QuestObjective& objective, const std::string& questTitle) {
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
    activeContext.subObjectiveIndex = subObjectiveIndex;
    activeContext.questTitle = questTitle;
    activeContext.puzzleKey = puzzleKey;

    const PuzzleType puzzleType = GetPuzzleTypeForObjective(questCharacter, objectiveIndex, subObjectiveIndex);
    activePuzzle = CreatePuzzle(puzzleType, questTitle, objective, questCharacter, objectiveIndex, subObjectiveIndex);
    if (!activePuzzle) {
        return false;
    }

    activePuzzle->SetSoundHook([this](const std::string& cue) {
        if (soundHook) {
            soundHook(cue);
        }
    });
    activePuzzle->SetConsequenceHook([this](const std::string& consequence) {
        if (consequenceHook) {
            consequenceHook(consequence);
        }
    });
    activePuzzle->Start();
    overlayAlpha = 0.0f;
    solvedMessage.clear();
    solvedMessageTimer = 0.0f;
    modalAge = 0.0f;
    lastPuzzleSnapshot = activePuzzle->SerializeState();
    showControls = (puzzleType == PuzzleType::JadeSymbolPuzzle);
    showHint = showControls;
    hintText = showControls ? activePuzzle->GetClueText() : std::string();
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
    modalAge = 0.0f;
    lastPuzzleSnapshot.clear();
    showHint = false;
    showControls = false;
    hintText.clear();
}

void PuzzleManager::Update(float dt, const InputContext& input) {
    if (activePuzzle) {
        modalAge += dt;
        overlayAlpha = std::min(1.0f, overlayAlpha + dt * 3.5f);
        activePuzzle->Update(dt, input);

        if (activePuzzle && activePuzzle->IsSolved()) {
            CompleteActivePuzzle();
        }
    } else {
        overlayAlpha = std::max(0.0f, overlayAlpha - dt * 2.5f);
    }

    if (solvedMessageTimer > 0.0f) {
        solvedMessageTimer = std::max(0.0f, solvedMessageTimer - dt);
    }

    if (input.IsActionPressed(InputAction::ToggleHint)) {
        showControls = !showControls;
        showHint = showControls;
        if (showControls && activePuzzle) {
            hintText = activePuzzle->GetClueText();
        }
    }

    if (input.IsActionPressed(InputAction::Cancel)) {
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
        const float centerX = static_cast<float>(screenWidth) * 0.5f;
        const float topY = static_cast<float>(screenHeight) - 100.0f;
        
        textRenderer.RenderText("◆ PUZZLE ◆", centerX - 200.0f, topY + 40.0f, 1.0f, glm::vec3(1.0f, 0.7f, 0.3f) * overlayAlpha, screenWidth, screenHeight);
        textRenderer.RenderText(activePuzzle->GetTitle(), centerX - 280.0f, topY, 1.55f, glm::vec3(1.0f, 0.95f, 0.8f) * overlayAlpha, screenWidth, screenHeight);
        textRenderer.RenderText(activePuzzle->GetClueText(), centerX - 280.0f, topY - 85.0f, 0.8f, glm::vec3(0.85f, 0.85f, 0.85f) * overlayAlpha, screenWidth, screenHeight);
        
        activePuzzle->Render(textRenderer, screenWidth, screenHeight, overlayAlpha);
    }

    if (!solvedMessage.empty() && solvedMessageTimer > 0.0f) {
        textRenderer.RenderText(solvedMessage, static_cast<float>(screenWidth) * 0.08f, static_cast<float>(screenHeight) * 0.22f, 0.50f, glm::vec3(1.0f, 0.25f, 0.25f) * std::min(1.0f, solvedMessageTimer / solvedMessageHold), screenWidth, screenHeight);
    }
}

void PuzzleManager::CancelActivePuzzle() {
    activePuzzle.reset();
    overlayAlpha = 0.0f;
    modalAge = 0.0f;
    lastPuzzleSnapshot.clear();
    showHint = false;
    showControls = false;
    hintText.clear();
}

void PuzzleManager::ResetSolvedState() {
    solvedPuzzleKeys.clear();
}

bool PuzzleManager::ConsumeSpawnRestartRequest() {
    const bool requested = spawnRestartRequested;
    spawnRestartRequested = false;
    return requested;
}

std::string PuzzleManager::SerializeState() const {
    nlohmann::json json;
    json["solved"] = std::vector<std::string>(solvedPuzzleKeys.begin(), solvedPuzzleKeys.end());
    return json.dump();
}

void PuzzleManager::DeserializeState(const std::string& state) {
    solvedPuzzleKeys.clear();
    if (state.empty()) return;
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
