#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_set>

#include "game/entities/Character.h"
#include "engine/input/InputContext.h"
#include "engine/renderer/Mesh.h"
#include "engine/renderer/Shader.h"
#include "game/puzzles/core/PuzzleBase.h"
#include "game/puzzles/PuzzleTypes.h"
#include "game/quest/Quest.h"

class TextRenderer;

class PuzzleManager {
public:
    using CompletionCallback = std::function<void(CharacterType characterType, int objectiveIndex)>;

    PuzzleManager();
    ~PuzzleManager();

    void Initialize();
    void SetCompletionCallback(CompletionCallback callback);
    void SetSoundHook(PuzzleBase::SoundHook hook);

    bool StartPuzzle(CharacterType questCharacter, int objectiveIndex, const QuestObjective& objective, const std::string& questTitle);
    bool StartPuzzle(CharacterType questCharacter, int objectiveIndex, int subObjectiveIndex, const QuestObjective& objective, const std::string& questTitle);
    void Update(float dt, const InputContext& input);
    void Render(TextRenderer& textRenderer, int screenWidth, int screenHeight) const;

    bool IsActive() const { return activePuzzle != nullptr; }
    bool IsSolved(const std::string& puzzleKey) const;
    void CancelActivePuzzle();
    void ResetSolvedState();
    bool ConsumeSpawnRestartRequest();

    std::string SerializeState() const;
    void DeserializeState(const std::string& state);

    std::string GetOverlayTitle() const;
    std::string GetOverlayClue() const;
    std::string GetSolvedMessage() const;

private:
    struct ActivePuzzleContext {
        CharacterType questCharacter = CharacterType::Boyd;
        int objectiveIndex = -1;
        int subObjectiveIndex = -1;
        std::string questTitle;
        std::string puzzleKey;
    };

    std::unique_ptr<PuzzleBase> activePuzzle;
    ActivePuzzleContext activeContext;
    std::unordered_set<std::string> solvedPuzzleKeys;
    CompletionCallback completionCallback;
    PuzzleBase::SoundHook soundHook;
    float overlayAlpha = 0.0f;
    float solvedMessageTimer = 0.0f;
    float solvedMessageHold = 1.25f;
    std::string solvedMessage;
    bool spawnRestartRequested = false;

    Mesh overlayMesh;
    mutable Shader overlayShader;
    bool overlayReady = false;
    // Hint UI state
    bool showHint = false;
    bool showControls = false;
    std::string hintText;
    float modalAge = 0.0f;
    std::string lastPuzzleSnapshot;

    static std::string MakePuzzleKey(CharacterType questCharacter, int objectiveIndex);
    static std::string ShiftCipher(const std::string& value, int offset);
    static std::array<std::string, 4> MakeCreepyOptions(const std::string& answer);
    static std::array<int, 4> MakeSequenceFromKey(const std::string& key);
    std::unique_ptr<PuzzleBase> CreatePuzzle(PuzzleType puzzleType,
                                             const std::string& questTitle,
                                             const QuestObjective& objective,
                                             CharacterType questCharacter,
                                             int objectiveIndex,
                                             int subObjectiveIndex) const;
    static PuzzleType GetPuzzleTypeForObjective(CharacterType questCharacter, int objectiveIndex, int subObjectiveIndex);
    void CompleteActivePuzzle();
};