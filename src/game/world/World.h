#pragma once

#include <vector>
#include <memory>
#include <string>
#include <array>

#include <glm/glm.hpp>

#include "engine/physics/CollisionWorld.h"
#include "game/entities/Enemy.h"
#include "game/entities/NPC.h"
#include "game/entities/Character.h"
#include "game/entities/Boyd.h"
#include "game/entities/Jade.h"
#include "game/entities/Tabitha.h"
#include "game/entities/Victor.h"
#include "game/entities/Sara.h"
#include "game/interactions/InteractionSystem.h"
#include "game/puzzles/core/PuzzleManager.h"
#include "game/quest/QuestSystem.h"
#include "game/runtime/EventBus.h"
#include "game/runtime/RuntimeProfiler.h"
#include "game/runtime/Timers.h"
#include "game/world/EntityManager.h"
#include "game/world/WorldManager.h"
#include "engine/audio/AudioManager.h"

class Shader;
class Mesh;
class TextRenderer;
class InputContext;

class Camera;
class MapManager;
class TerrainRenderer;

struct LocationZone {
    std::string id;
    glm::vec3 center = glm::vec3(0.0f);
    float radius = 4.0f;
};

struct ShelterZone {
    std::string id;
    glm::vec3 center = glm::vec3(0.0f);
    float radius = 4.0f;
    bool talismanPlaced = true;
    bool sealed = true;
};

struct CharacterSimState {
    glm::vec3 position = glm::vec3(0.0f);
    float health = 100.0f;
};

struct WorldSaveState {
    float worldClock = 0.0f;
    int activeCharacterIndex = 0;
    bool playerKilled = false;
    std::string questState;
    std::string puzzleState;
    std::array<CharacterSimState, 5> characters;
    std::string dialogueRelationships; // JSON blob for relationships/memory
};

class World {
public:
    World();

    void Initialize();
    void Update(const Camera& camera, float dt);
    void Render(const Camera& camera, float aspectRatio);

    Character* GetActiveCharacter();
    const Character* GetActiveCharacter() const;
    void SwitchCharacter(int index);
    QuestSystem* GetQuestSystem() { return questSystem.get(); }
    bool TryActiveCharacterInteraction();
    bool TryActiveCharacterPickup();
    std::string GetInteractionPrompt() const;
    bool NearestInteractionIsPickup() const;
    void SetActiveQuest(CharacterType characterType);
    void AbandonActiveQuest();
    bool IsPuzzleActive() const;
    DayCyclePhase GetCyclePhase() const { return cyclePhase; }
    bool IsDayTime() const { return cyclePhase == DayCyclePhase::Morning || cyclePhase == DayCyclePhase::Afternoon; }
    bool IsSunsetTime() const { return cyclePhase == DayCyclePhase::Sunset; }
    bool IsNightTime() const { return nightTime; }
    void UpdatePuzzle(float dt, const InputContext& input);
    void RenderPuzzleOverlay(TextRenderer& textRenderer, int screenWidth, int screenHeight) const;
    void RenderNarrativeOverlays(TextRenderer& textRenderer, int screenWidth, int screenHeight) const;
    CharacterType GetActiveQuestCharacter() const { return activeQuestCharacter; }
    bool HasActiveQuest() const { return hasActiveQuest; }
    std::string GetLastInteractionFeedback() const { return lastInteractionFeedback; }
    float GetLastInteractionFeedbackTime() const { return lastInteractionFeedbackTimer.Remaining(); }
    bool HasStoryFlag(const std::string& flag) const;
    std::string GetLastMonsterScream() const { return lastMonsterScream; }
    float GetMonsterScreamDisplayTime() const { return monsterScreamDisplayTimer.Remaining(); }
    float GetLastDamageAmount() const { return lastDamageAmount; }
    float GetLastDamageDisplayTime() const { return lastDamageDisplayTimer.Remaining(); }
    std::string GetLastNpcDialogue() const { return lastNpcDialogue; }
    float GetNpcDialogueDisplayTime() const { return npcDialogueDisplayTimer.Remaining(); }
    std::string GetQuestHelperText() const;
    std::string GetQuestWaypointText() const;  // Returns compass waypoint info
    WorldSaveState CaptureSaveState() const;
    void RestoreSaveState(const WorldSaveState& state);
    bool ConsumeSpawnRestartRequest();
    void RestartFromSpawn();
    bool SaveToFile(const std::string& path) const;
    bool LoadFromFile(const std::string& path);

    // Access to audio manager for global systems (transitions, UI)
    AudioManager* GetAudioManager() { return audioManager.get(); }

private:
    MapManager* mapManager = nullptr;
     void ApplyPuzzleConsequence(const std::string& consequence);
     void EnsureTabithaWhisperRouteNodes();
    TerrainRenderer* terrain = nullptr;
    std::vector<NPC> npcs;
    std::vector<Enemy> enemies;
    std::vector<LocationZone> storyLocations;
    std::vector<ShelterZone> shelterZones;
    
    // Multi-character system
    std::vector<std::unique_ptr<Character>> characters;  // 5 characters (Boyd, Jade, Tabitha, Victor, Sara)
    int activeCharacterIndex = 0;  // Which character is active
    
    // Quest system
    std::unique_ptr<QuestSystem> questSystem;
    
    // Active quest tracking
    CharacterType activeQuestCharacter = CharacterType::Boyd;
    bool hasActiveQuest = false;
    std::string lastInteractionFeedback;
    DurationTimer lastInteractionFeedbackTimer;

    // Interaction system
    InteractionSystem interactionSystem;
    PuzzleManager puzzleManager;
    EntityManager entityManager;
    WorldManager worldManager;
    EventBus eventBus;
    RuntimeProfiler runtimeProfiler;
    std::unique_ptr<AudioManager> audioManager;
    
    float worldClock = 0.0f;
    DayCyclePhase cyclePhase = DayCyclePhase::Morning;
    bool nightTime = false;
    bool playerKilled = false;
    bool hasPreviousActivePosition = false;
    glm::vec3 previousActivePosition = glm::vec3(0.0f);
    std::array<float, 5> offscreenStoryTimers = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    std::array<float, 5> characterDamageCooldowns = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};  // Damage cooldown per character

    CollisionWorld collisionWorld;
    
    // Monster and damage feedback tracking
    std::string lastMonsterScream;
    DurationTimer monsterScreamDisplayTimer;
    DurationTimer lastDamageDisplayTimer;
    float lastDamageAmount = 0.0f;
    std::string lastNpcDialogue;
    DurationTimer npcDialogueDisplayTimer;
    std::vector<std::array<float, 5>> npcTalkCooldowns;
    std::vector<float> npcPanicCooldowns;
    std::vector<float> npcNeighborTalkCooldowns;
    std::array<float, 5> characterMonsterInteractionCooldowns = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    std::array<float, 5> characterReactionCooldowns = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    int debugFrameCounter = 0;
    WorldSaveState initialSpawnState;
    bool hasInitialSpawnState = false;
    bool spawnRestartRequested = false;
        float tabithaWhisperTempRouteTimer = 0.0f;
        float tabithaWhisperFalseChamberTimer = 0.0f;
    static constexpr float kScreamDisplayDuration = 2.5f;
    static constexpr float kDamageDisplayDuration = 1.8f;
    static constexpr float kNpcDialogueDisplayDuration = 4.5f;

    void InitializeCharacters();
    void InitializePlaceholderWorldRules();
    void EnsureNpcDialogueCooldowns();
    bool TryNpcDialogue(Character& character, bool explicitInteraction);
    void UpdateTimeOfDay(float dt);
    void UpdateWorldSystemsPhase(float dt);
    void UpdateTimersAndCooldownsPhase(float dt);
    void UpdateQuestAndInteractionPhase(float dt);
    bool HandleInteractionOutcome(Character& activeChar, bool didInteract, const char* noTargetLog);
    void UpdateOffscreenCharacters(float dt);
    bool IsProtectedByShelter(const glm::vec3& position) const;
    glm::vec3 GetNearestShelterCenter(const glm::vec3& position) const;
    glm::vec3 GetCharacterStoryGoal(CharacterType type) const;
    bool MoveCharacterToward(Character& character, const glm::vec3& target, float dt);
    void TryAdvanceOffscreenStory(Character& character, std::size_t index, float dt, bool atGoal);
    void RenderInteriorZoneBounds(const Camera& camera, float aspectRatio) const;
};
