#pragma once

#include <vector>
#include "game/entities/Player.h"
#include "game/entities/Door.h"
#include "engine/physics/CollisionWorld.h"
#include "game/quest/QuestSystem.h"
#include "game/puzzles/core/PuzzleManager.h"
#include "game/interactions/InteractionSystem.h"
#include "engine/input/InputContext.h"

#include "game/entities/Boyd.h"
#include "game/entities/Jade.h"
#include "game/entities/Tabitha.h"
#include "game/entities/Victor.h"
#include "game/entities/Sara.h"
#include "engine/audio/AudioManager.h"

class Camera;
class DayNightCycle;

class World {
public:
    World();

    void Initialize();
    void Update(const Camera& camera, float dt, InputManager& input);
    void Render(const Camera& camera, float aspectRatio);
    void RenderObjects(const Camera& camera, float aspectRatio, const DayNightCycle& dayNight, float fogDensity);
    void TryInteract();
    void TryExit();

    Character& GetPlayer() { return *characters[activeCharacterIndex]; }
    const Character& GetPlayer() const { return *characters[activeCharacterIndex]; }
    Character* GetActiveCharacter() { return characters[activeCharacterIndex].get(); }
    void SwitchCharacter(int index);
    CollisionWorld* GetCollisionWorld() { return &collisionWorld; }
    bool IsInsideBuilding() const { return isInsideBuilding; }
    
    QuestSystem* GetQuestSystem() { return questSystem.get(); }
    PuzzleManager* GetPuzzleManager() { return &puzzleManager; }
    InteractionSystem* GetInteractionSystem() { return &interactionSystem; }
    AudioManager* GetAudioManager() { return audioManager.get(); }
    std::vector<std::unique_ptr<Character>>& GetCharacters() { return characters; }
    const std::vector<std::unique_ptr<Character>>& GetCharacters() const { return characters; }
 
    // HUD Helpers
    std::string GetInteractionPrompt() const;
    bool NearestInteractionIsPickup() const;
    std::string GetQuestHelperText() const;
    std::string GetQuestWaypointText() const;
    float GetNpcDialogueDisplayTime() const { return npcDialogueDisplayTime; }
    std::string GetLastNpcDialogue() const { return lastNpcDialogue; }
    float GetMonsterScreamDisplayTime() const { return monsterScreamDisplayTime; }
    std::string GetLastMonsterScream() const { return lastMonsterScream; }
    float GetLastDamageDisplayTime() const { return lastDamageDisplayTime; }
    float GetLastDamageAmount() const { return lastDamageAmount; }
    float GetLastInteractionFeedbackTime() const { return lastInteractionFeedbackTime; }
    std::string GetLastInteractionFeedback() const { return lastInteractionFeedback; }
    
    bool IsPuzzleActive() const;
    void UpdatePuzzle(float dt, const InputContext& input);
    bool HasActiveQuest() const;
    CharacterType GetActiveQuestCharacter() const;
    
    enum class DayCyclePhase { Morning, Afternoon, Sunset, Night };
    DayCyclePhase GetCyclePhase() const;
    
    void RenderNarrativeOverlays(TextRenderer& renderer, int width, int height);
    void RenderPuzzleOverlay(TextRenderer& renderer, int width, int height);
    bool ConsumeSpawnRestartRequest();

private:
    CollisionWorld collisionWorld;
    std::vector<Door> doors;
    
    bool isInsideBuilding = false;
    glm::vec3 previousOutsidePosition;

    std::unique_ptr<QuestSystem> questSystem;
    PuzzleManager puzzleManager;
    InteractionSystem interactionSystem;
    InputContext inputContext;

    std::vector<std::unique_ptr<Character>> characters;
    int activeCharacterIndex = 0;
    float worldClock = 0.0f;
    std::unique_ptr<AudioManager> audioManager;
 
    float npcDialogueDisplayTime = 0.0f;
    std::string lastNpcDialogue;
    float monsterScreamDisplayTime = 0.0f;
    std::string lastMonsterScream;
    float lastDamageDisplayTime = 0.0f;
    float lastDamageAmount = 0.0f;
    float lastInteractionFeedbackTime = 0.0f;
    std::string lastInteractionFeedback;
    bool spawnRestartRequested = false;
};