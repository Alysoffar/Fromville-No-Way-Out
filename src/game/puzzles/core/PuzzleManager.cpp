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

#include "engine/core/InputManager.h"
#include "engine/renderer/Mesh.h"
#include "engine/renderer/Shader.h"
#include "engine/renderer/TextRenderer.h"
#include "game/puzzles/logic/Jade/JadeDebatePuzzle.h"
#include "game/puzzles/logic/Boyed/CipherDecodePuzzle.h"
#include "game/puzzles/logic/Boyed/EvidenceBoardPuzzle.h"
#include "game/puzzles/logic/Boyed/ConnectTheCluesPuzzle.h"
#include "game/puzzles/logic/Boyed/CultDebatePuzzle.h"
#include "game/puzzles/logic/Boyed/LedgerRotationPuzzle.h"
#include "game/puzzles/logic/Boyed/MaraTrustDialoguePuzzle.h"
#include "game/puzzles/logic/Boyed/RitualInferencePuzzle.h"
#include "game/puzzles/logic/Jade/JadeSymbolPuzzle.h"
#include "game/puzzles/logic/Jade/JadeSymbolVaultPuzzle.h"
#include "game/puzzles/logic/Tabitha/HollowWallPuzzle.h"
#include "game/puzzles/logic/Victor/GhostWitnessPuzzle.h"
#include "game/puzzles/logic/Boyed/SequenceMemoryPuzzle.h"
#include "game/puzzles/logic/Boyed/SymbolMatchPuzzle.h"
#include "game/puzzles/logic/Tabitha/TabithaTunnelPuzzle.h"
#include "game/puzzles/logic/Tabitha/TabithaTunnelMapPuzzle.h"
#include "game/puzzles/logic/Tabitha/HollowWallPuzzle.h"
#include "game/puzzles/logic/Tabitha/TabithaEchoDialoguePuzzle.h"
#include "game/puzzles/logic/Tabitha/TabithaWhisperRoutePuzzle.h"
#include "game/puzzles/logic/Victor/VictorMemoryPuzzle.h"
#include "game/puzzles/logic/Victor/VictorMemoryTimelinePuzzle.h"
#include "game/puzzles/logic/Victor/GhostWitnessPuzzle.h"
#include "game/puzzles/logic/Victor/VictorEmptyChairPuzzle.h"
#include "game/puzzles/logic/Victor/VictorMirrorCollapsePuzzle.h"
#include "game/puzzles/logic/Victor/VictorSilentProcessionPuzzle.h"
#include "game/puzzles/logic/Boyed/WordScramblePuzzle.h"
#include "game/story/StoryManager.h"

// Sara puzzle headers removed

#include "game/puzzles/logic/Tabitha/HollowWallPuzzle.h"
#include "game/puzzles/logic/Victor/GhostWitnessPuzzle.h"

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
        if (objectiveIndex == 0) {
            return PuzzleType::MaraTrustDialogue;
        }

        if (objectiveIndex == 1 && subObjectiveIndex == 1) {
            return PuzzleType::LedgerRotation;
        }

        if (objectiveIndex == 2) {
            return PuzzleType::EvidenceBoard;
        }

        if (objectiveIndex == 3) {
            return PuzzleType::CultDebate;
        }

        if (objectiveIndex == 4) {
            return PuzzleType::RitualInference;
        }
    }

    if (questCharacter == CharacterType::Jade) {
        // Make symbol decoding the opening objective and the debate (Man in Yellow) the final confrontation
        if (objectiveIndex == 0) return PuzzleType::JadeSymbolPuzzle;
        if (objectiveIndex == 1) return PuzzleType::JadeSymbolVaultPuzzle;
        if (objectiveIndex == 4) return PuzzleType::JadeDebatePuzzle; // final confrontation
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
        if (objectiveIndex == 2) return PuzzleType::TabithaWhisperRoutePuzzle; // 3rd (whisper-route buildup)
        if (objectiveIndex == 3) return PuzzleType::TabithaEchoDialoguePuzzle; // 4th now different
        return PuzzleType::HollowWallPuzzle; // subsequent objectives remain ankouhi guessing
    }

    if (questCharacter == CharacterType::Victor) {
        if (objectiveIndex == 0) return PuzzleType::GhostWitnessPuzzle;
        if (objectiveIndex == 1) return PuzzleType::VictorEmptyChairPuzzle;
        if (objectiveIndex == 2) return PuzzleType::VictorMirrorCollapsePuzzle;
        if (objectiveIndex == 3) return PuzzleType::VictorSilentProcessionPuzzle;
        return PuzzleType::VictorMemoryPuzzle;
    }

    // Sara puzzles removed; fall through to default assignment

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
    (void)subObjectiveIndex;
    const std::string key = MakePuzzleKey(questCharacter, objectiveIndex);
    const std::string clue = objective.environmentalClues.empty() ? objective.description : objective.environmentalClues.front();

    switch (puzzleType) {
        case PuzzleType::MaraTrustDialogue:
            return std::make_unique<MaraTrustDialoguePuzzle>(questTitle, clue, "SOLVED: Mara trusts you and reveals the first lead.");
        case PuzzleType::LedgerRotation:
            return std::make_unique<LedgerRotationPuzzle>(questTitle, clue, "SOLVED: the ledger reveals the next shadow path.");
        case PuzzleType::EvidenceBoard:
            return std::make_unique<EvidenceBoardPuzzle>(questTitle, clue, "SOLVED: the evidence board maps the cult territory.");
        case PuzzleType::CultDebate:
            return std::make_unique<CultDebatePuzzle>(questTitle, clue, "SOLVED: the leader breaks under pressure.");
        case PuzzleType::RitualInference:
            return std::make_unique<RitualInferencePuzzle>(questTitle, clue, "SOLVED: the ritual pattern collapses.");
        case PuzzleType::JadeDebatePuzzle:
            return std::make_unique<JadeDebatePuzzle>();
        case PuzzleType::JadeSymbolPuzzle:
            return std::make_unique<JadeSymbolPuzzle>(questTitle, clue, "SOLVED: the false translation shatters.");
        case PuzzleType::TabithaTunnelPuzzle:
            return std::make_unique<TabithaTunnelPuzzle>(questTitle, clue, "SOLVED: the mimic voice is exposed.");
        case PuzzleType::VictorMemoryPuzzle:
            return std::make_unique<VictorMemoryPuzzle>(questTitle, clue, "SOLVED: the memory loop stops lying.");
        // Sara puzzles removed
        case PuzzleType::JadeSymbolVaultPuzzle:
            return std::make_unique<JadeSymbolVaultPuzzle>(questTitle, clue, "SOLVED: the living symbols are contained.");
        case PuzzleType::TabithaTunnelMapPuzzle:
            return std::make_unique<TabithaTunnelMapPuzzle>(questTitle, clue, "SOLVED: the pressure routes hold and the flood recedes.");
        case PuzzleType::TabithaWhisperRoutePuzzle:
            return std::make_unique<TabithaWhisperRoutePuzzle>(questTitle, clue, "SOLVED: the whispers point the way and Tabitha learns their language of fear.");
        case PuzzleType::TabithaEchoDialoguePuzzle:
            return std::make_unique<TabithaEchoDialoguePuzzle>(questTitle, clue, "SOLVED: Tabitha reads the echoes and finds a new way forward.");
        case PuzzleType::VictorMemoryTimelinePuzzle:
            return std::make_unique<VictorMemoryTimelinePuzzle>(questTitle, clue, "SOLVED: the missing person is recovered from absence.");
        // Sara final judgment puzzle removed
        case PuzzleType::HollowWallPuzzle:
            return std::make_unique<HollowWallPuzzle>(questTitle, clue, "SOLVED: the wall gives up its hidden chamber.");
        case PuzzleType::GhostWitnessPuzzle:
            return std::make_unique<GhostWitnessPuzzle>(questTitle, clue, "SOLVED: the ghost witness cannot keep the lie alive.");
        case PuzzleType::VictorEmptyChairPuzzle:
            return std::make_unique<VictorEmptyChairPuzzle>(questTitle, clue, "SOLVED: the erased person partially manifests.");
        case PuzzleType::VictorMirrorCollapsePuzzle:
            return std::make_unique<VictorMirrorCollapsePuzzle>(questTitle, clue, "SOLVED: the mirrors surrender the real memory.");
        case PuzzleType::VictorSilentProcessionPuzzle:
            return std::make_unique<VictorSilentProcessionPuzzle>(questTitle, clue, "SOLVED: the procession reveals the ritual route.");
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
        } else {
            std::cout << "[PuzzleSound] " << cue << "\n";
        }
    });
    activePuzzle->SetConsequenceHook([this](const std::string& consequence) {
        if (consequenceHook) {
            consequenceHook(consequence);
        } else {
            std::cout << "[PuzzleConsequence] " << consequence << "\n";
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
    StoryManager::Instance().ResetHintTimer();
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

        if (activePuzzle && activePuzzle->IsCorrupted()) {
            solvedMessage = "The vision collapses. Stay with Victor and try that memory again.";
            solvedMessageTimer = solvedMessageHold;
            activePuzzle.reset();
            overlayAlpha = 0.0f;
            modalAge = 0.0f;
            lastPuzzleSnapshot.clear();
            showHint = false;
            showControls = false;
            hintText.clear();
            return;
        }

        const std::string snapshot = activePuzzle->SerializeState();
        if (snapshot != lastPuzzleSnapshot) {
            lastPuzzleSnapshot = snapshot;
            StoryManager::Instance().ResetHintTimer();
        } else {
            StoryManager::Instance().AdvanceHintTimer(dt);
        }

        if (StoryManager::Instance().ShouldDecayHintText()) {
            hintText = StoryManager::Instance().GetCrypticHintForObjective(activeContext.objectiveIndex, activePuzzle->GetTitle());
        } else if (showHint) {
            hintText = activePuzzle->GetClueText();
        }

        if (activePuzzle->IsSolved()) {
            CompleteActivePuzzle();
        }
    } else {
        overlayAlpha = std::max(0.0f, overlayAlpha - dt * 2.5f);
    }

    if (solvedMessageTimer > 0.0f) {
        solvedMessageTimer = std::max(0.0f, solvedMessageTimer - dt);
    }

    // Toggle the puzzle help / controls sheet with H
    if (input.IsActionPressed(InputAction::ToggleHint)) {
        showControls = !showControls;
        showHint = showControls;
        if (showControls && activePuzzle) {
            hintText = activePuzzle->GetClueText();
        } else {
            hintText.clear();
        }
    }

    // Allow closing puzzle with ESC
    if (input.IsActionPressed(InputAction::Cancel)) {
        CancelActivePuzzle();
    }
}

void PuzzleManager::Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, bool isNight, float nightTimer) const {
    const float previousTextScale = textRenderer.GetScaleMultiplier();
    const float previousMinimumScale = textRenderer.GetMinimumScale();
    if (activePuzzle || solvedMessageTimer > 0.0f) {
        textRenderer.SetScaleMultiplier(1.0f);
        textRenderer.SetMinimumScale(0.62f);
    }

    if (overlayReady && overlayAlpha > 0.0f) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);

        overlayShader.Bind();
        overlayShader.SetFloat("uAlpha", 0.88f * overlayAlpha);
        overlayShader.SetFloat("uInvestigationMode", activePuzzle ? 1.0f : 0.0f);
        overlayShader.SetFloat("uTerrorLevel", StoryManager::Instance().GetGlobalTension());
        overlayShader.SetFloat("uModalAge", modalAge);
        // Character-specific overlay controls (e.g. Jade symbol visibility)
        float symbolVis = 0.0f;
        float sketchOverlayAlpha = 0.0f;
        float invisibilityDistortion = 0.0f;
        if (activePuzzle) {
            const PuzzleType ptype = GetPuzzleTypeForObjective(activeContext.questCharacter, activeContext.objectiveIndex, activeContext.subObjectiveIndex);
            if (ptype == PuzzleType::JadeSymbolPuzzle) {
                symbolVis = 0.0f;
            } else if (ptype == PuzzleType::VictorMemoryPuzzle) {
                sketchOverlayAlpha = 0.5f * overlayAlpha;
            }
        }
        overlayShader.SetFloat("u_SymbolVisibility", symbolVis);
        overlayShader.SetFloat("u_SketchOverlayAlpha", sketchOverlayAlpha);
        overlayShader.SetFloat("u_InvisibilityDistortion", invisibilityDistortion);
        overlayShader.SetVec3("uTint", glm::vec3(0.015f, 0.015f, 0.022f));
        overlayMesh.Draw();
        overlayShader.Unbind();

        glEnable(GL_DEPTH_TEST);
    }

    if (activePuzzle) {
        const float centerX = static_cast<float>(screenWidth) * 0.5f;
        const float topY = static_cast<float>(screenHeight) - 80.0f;
        const std::string title = activePuzzle->GetTitle();
        const bool decayHints = StoryManager::Instance().ShouldDecayHintText();
        const std::string objective = decayHints && !hintText.empty() ? hintText : activePuzzle->GetClueText();
        const float jitter = decayHints ? std::sin(modalAge * 17.0f) * 6.0f : 0.0f;
        const PuzzleType puzzleType = GetPuzzleTypeForObjective(activeContext.questCharacter, activeContext.objectiveIndex, activeContext.subObjectiveIndex);

        // Predecessor premium colors
        const glm::vec3 goldColor(1.0f, 0.72f, 0.18f);
        const glm::vec3 whiteColor(0.95f, 0.95f, 0.92f);
        const glm::vec3 greyColor(0.72f, 0.75f, 0.78f);
        const glm::vec3 mutedColor(0.42f, 0.44f, 0.46f);
        const glm::vec3 dangerColor(0.92f, 0.32f, 0.32f);
        const glm::vec3 successColor(0.38f, 0.92f, 0.48f);

        // 1. TOP HEADER PANEL (with diamonds and centered text)
        std::string headerLabel = "♦  F R O M V I L L E   I N V E S T I G A T I O N  ♦";
        float headerX = centerX - (static_cast<float>(headerLabel.length()) * 4.4f);
        textRenderer.RenderText(headerLabel, headerX + jitter, topY + 45.0f, 0.72f, goldColor * overlayAlpha, screenWidth, screenHeight);
        
        std::string topDivider = "──────────────────────────────────────────────────────";
        float divX = centerX - (static_cast<float>(topDivider.length()) * 4.4f);
        textRenderer.RenderText(topDivider, divX + jitter, topY + 30.0f, 0.72f, mutedColor * overlayAlpha, screenWidth, screenHeight);

        // Centered Puzzle Name (huge & elegant)
        float titleX = centerX - (static_cast<float>(title.length()) * 6.4f);
        textRenderer.RenderText(title, titleX + jitter, topY - 5.0f, 1.25f, whiteColor * overlayAlpha, screenWidth, screenHeight);

        // 2. NIGHT TIMER (if active)
        if (isNight) {
            float remaining = std::max(0.0f, 60.0f - nightTimer);
            std::ostringstream timeStream;
            timeStream << std::fixed << std::setprecision(1) << remaining << "s";
            std::string timerText = "♦ BREACH TIME REMAINING: " + timeStream.str() + " ♦";
            glm::vec3 timerCol = (remaining < 15.0f) ? dangerColor : goldColor;
            
            float timerX = centerX - (static_cast<float>(timerText.length()) * 5.2f);
            textRenderer.RenderText(timerText, timerX + jitter, topY - 42.0f, 0.82f, timerCol * overlayAlpha, screenWidth, screenHeight);

            // Timer Progress Bar
            int totalSegments = 24;
            int filledSegments = static_cast<int>((remaining / 60.0f) * totalSegments);
            filledSegments = std::clamp(filledSegments, 0, totalSegments);
            std::string progressStr = "[" + std::string(filledSegments, '=') + std::string(totalSegments - filledSegments, '.') + "]";
            float progressX = centerX - (static_cast<float>(progressStr.length()) * 5.0f);
            textRenderer.RenderText(progressStr, progressX + jitter, topY - 62.0f, 0.78f, timerCol * overlayAlpha, screenWidth, screenHeight);

            if (remaining < 15.0f) {
                std::string warnMsg = "[ CRITICAL WARNING: MONSTERS OUTSIDE ARE BREAKING IN! ]";
                float warnX = centerX - (static_cast<float>(warnMsg.length()) * 4.2f);
                float blink = 0.5f + 0.5f * std::sin(modalAge * 14.0f);
                textRenderer.RenderText(warnMsg, warnX + jitter, topY - 82.0f, 0.58f, dangerColor * blink * overlayAlpha, screenWidth, screenHeight);
            }
        }

        // 3. LEFT PANEL (Intel, Lore, and Objective metadata)
        float leftX = 72.0f;
        float leftY = static_cast<float>(screenHeight) - 220.0f;
        textRenderer.RenderText("◇ INTEL & METADATA", leftX, leftY, 0.54f, goldColor * overlayAlpha, screenWidth, screenHeight);
        textRenderer.RenderText("──────────────────────", leftX, leftY - 14.0f, 0.50f, mutedColor * overlayAlpha, screenWidth, screenHeight);
        
        // Clean wrapped/spaced objective text
        textRenderer.RenderText("INSTRUCTION CLUE:", leftX, leftY - 40.0f, 0.48f, goldColor * overlayAlpha, screenWidth, screenHeight);
        
        // Render objective (wrap slightly or display cleanly)
        if (objective.length() > 34) {
            std::string line1 = objective.substr(0, 32) + "...";
            std::string line2 = "..." + objective.substr(32);
            textRenderer.RenderText(line1, leftX, leftY - 66.0f, 0.44f, whiteColor * overlayAlpha, screenWidth, screenHeight);
            textRenderer.RenderText(line2, leftX, leftY - 88.0f, 0.44f, whiteColor * overlayAlpha, screenWidth, screenHeight);
        } else {
            textRenderer.RenderText(objective, leftX, leftY - 66.0f, 0.44f, whiteColor * overlayAlpha, screenWidth, screenHeight);
        }

        // 4. RIGHT PANEL (High-fidelity Keybind list, aligned to right column)
        float rightX = static_cast<float>(screenWidth) - 340.0f;
        float rightY = static_cast<float>(screenHeight) - 220.0f;
        textRenderer.RenderText("◇ INVESTIGATION KEYMAP", rightX, rightY, 0.54f, goldColor * overlayAlpha, screenWidth, screenHeight);
        textRenderer.RenderText("──────────────────────", rightX, rightY - 14.0f, 0.50f, mutedColor * overlayAlpha, screenWidth, screenHeight);

        float bindY = rightY - 40.0f;
        textRenderer.RenderText("[1 - 4] Choose Option", rightX, bindY, 0.48f, whiteColor * overlayAlpha, screenWidth, screenHeight);
        textRenderer.RenderText("[ENTER] Submit Decipher", rightX, bindY - 24.0f, 0.48f, whiteColor * overlayAlpha, screenWidth, screenHeight);
        textRenderer.RenderText("[ESC]   Steady Mind (Exit)", rightX, bindY - 48.0f, 0.48f, whiteColor * overlayAlpha, screenWidth, screenHeight);
        textRenderer.RenderText("[R]     Reset Board", rightX, bindY - 72.0f, 0.48f, whiteColor * overlayAlpha, screenWidth, screenHeight);
        textRenderer.RenderText("[CTRL]  Toggle Controls Overlay", rightX, bindY - 96.0f, 0.48f, whiteColor * overlayAlpha, screenWidth, screenHeight);

        if (showControls) {
            float helpY = rightY - 158.0f;
            textRenderer.RenderText("◇ CHARACTER NAVIGATION", rightX, helpY, 0.50f, goldColor * overlayAlpha, screenWidth, screenHeight);
            textRenderer.RenderText("[WASD]  Move Character", rightX, helpY - 24.0f, 0.44f, greyColor * overlayAlpha, screenWidth, screenHeight);
            textRenderer.RenderText("[SPACE] Jump Obstacle", rightX, helpY - 44.0f, 0.44f, greyColor * overlayAlpha, screenWidth, screenHeight);
            textRenderer.RenderText("[C]     Crouch / Stealth", rightX, helpY - 64.0f, 0.44f, greyColor * overlayAlpha, screenWidth, screenHeight);
            textRenderer.RenderText("[1 - 5] Swap Active Hero", rightX, helpY - 84.0f, 0.44f, greyColor * overlayAlpha, screenWidth, screenHeight);
        }

        // Draw the puzzle itself (which handles its own beautiful centered layout)
        activePuzzle->Render(textRenderer, screenWidth, screenHeight, overlayAlpha);

        // Unified selector UI (highly optimized, centered selector brackets)
        const int sel = activePuzzle->GetSelectedIndex();
        if (sel >= 1 && sel <= 3) {
            const float selY = static_cast<float>(screenHeight) * 0.58f;
            const float selX = static_cast<float>(screenWidth) * 0.5f;
            const float boxSpacing = 120.0f;
            for (int i = 1; i <= 3; ++i) {
                const bool isSel = (i == sel);
                const glm::vec3 boxBg = isSel ? glm::vec3(0.24f, 0.18f, 0.08f) : glm::vec3(0.08f, 0.09f, 0.11f);
                const glm::vec3 boxFg = isSel ? goldColor : greyColor;
                const float x = selX + (static_cast<float>(i) - 2.0f) * boxSpacing;
                
                textRenderer.RenderText("[ ", x - 48.0f, selY + 18.0f, 2.4f, boxBg * overlayAlpha, screenWidth, screenHeight);
                std::ostringstream label;
                label << i;
                textRenderer.RenderText(label.str(), x - 8.0f, selY - 12.0f, 2.6f, boxFg * overlayAlpha, screenWidth, screenHeight);
                textRenderer.RenderText(" ]", x + 32.0f, selY + 18.0f, 2.4f, boxBg * overlayAlpha, screenWidth, screenHeight);
            }
            textRenderer.RenderText("Press number keys to swap your selection", selX - 160.0f, selY + 90.0f, 0.48f, greyColor * overlayAlpha, screenWidth, screenHeight);
        }

        if (overlayAlpha > 0.0f) {
            const std::string solvedBanner = "★ PUZZLE SOLVED ★";
            if (solvedMessageTimer > 0.0f) {
                textRenderer.RenderText(solvedBanner, centerX - 180.0f, static_cast<float>(screenHeight) * 0.45f, 1.6f, successColor, screenWidth, screenHeight);
            }
        }
    }

    if (!solvedMessage.empty() && solvedMessageTimer > 0.0f) {
        textRenderer.RenderText(solvedMessage, static_cast<float>(screenWidth) * 0.08f, static_cast<float>(screenHeight) * 0.22f, 0.50f, glm::vec3(1.0f, 0.25f, 0.25f) * std::min(1.0f, solvedMessageTimer / solvedMessageHold), screenWidth, screenHeight);
    }

    textRenderer.SetScaleMultiplier(previousTextScale);
    textRenderer.SetMinimumScale(previousMinimumScale);
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
