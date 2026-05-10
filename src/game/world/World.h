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
#include "game/quest/QuestSystem.h"

class Shader;
class Mesh;

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
    std::array<CharacterSimState, 5> characters;
};

class World {
public:
    World();

    void Initialize();
    void Update(const Camera& camera, float dt);
    void Render(const Camera& camera, float aspectRatio);

    Character* GetActiveCharacter() { return characters[activeCharacterIndex].get(); }
    void SwitchCharacter(int index);
    QuestSystem* GetQuestSystem() { return questSystem.get(); }
    bool TryActiveCharacterInteraction();
    std::string GetInteractionPrompt() const;
    void SetActiveQuest(CharacterType characterType);
    void AbandonActiveQuest();
    CharacterType GetActiveQuestCharacter() const { return activeQuestCharacter; }
    bool HasActiveQuest() const { return hasActiveQuest; }
    std::string GetLastInteractionFeedback() const { return lastInteractionFeedback; }
    float GetLastInteractionFeedbackTime() const { return lastInteractionFeedbackTime; }
    bool HasStoryFlag(const std::string& flag) const;
    std::string GetLastMonsterScream() const { return lastMonsterScream; }
    float GetMonsterScreamDisplayTime() const { return monsterScreamDisplayTime; }
    float GetLastDamageAmount() const { return lastDamageAmount; }
    float GetLastDamageDisplayTime() const { return lastDamageTime; }
    std::string GetLastNpcDialogue() const { return lastNpcDialogue; }
    float GetNpcDialogueDisplayTime() const { return npcDialogueDisplayTime; }
    std::string GetQuestHelperText() const;
    WorldSaveState CaptureSaveState() const;
    void RestoreSaveState(const WorldSaveState& state);
    bool SaveToFile(const std::string& path) const;
    bool LoadFromFile(const std::string& path);

private:
    MapManager* mapManager = nullptr;
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
    float lastInteractionFeedbackTime = 0.0f;

    // Interaction system
    InteractionSystem interactionSystem;
    
    float worldClock = 0.0f;
    bool nightTime = false;
    bool playerKilled = false;
    bool hasPreviousActivePosition = false;
    glm::vec3 previousActivePosition = glm::vec3(0.0f);
    std::array<float, 5> offscreenStoryTimers = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    std::array<float, 5> characterDamageCooldowns = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};  // Damage cooldown per character

    CollisionWorld collisionWorld;
    
    // Monster and damage feedback tracking
    std::string lastMonsterScream;
    float monsterScreamDisplayTime = 0.0f;
    float lastDamageTime = 0.0f;
    float lastDamageAmount = 0.0f;
    std::string lastNpcDialogue;
    float npcDialogueDisplayTime = 0.0f;
    std::vector<std::array<float, 5>> npcTalkCooldowns;
    std::vector<float> npcPanicCooldowns;
    std::vector<float> npcNeighborTalkCooldowns;
    std::array<float, 5> characterMonsterInteractionCooldowns = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    static constexpr float kScreamDisplayDuration = 2.5f;
    static constexpr float kDamageDisplayDuration = 1.8f;
    static constexpr float kNpcDialogueDisplayDuration = 4.5f;

    void InitializeCharacters();
    void InitializePlaceholderWorldRules();
    void EnsureNpcDialogueCooldowns();
    bool TryNpcDialogue(Character& character, bool explicitInteraction);
    void UpdateTimeOfDay(float dt);
    void UpdateOffscreenCharacters(float dt);
    bool IsProtectedByShelter(const glm::vec3& position) const;
    glm::vec3 GetNearestShelterCenter(const glm::vec3& position) const;
    glm::vec3 GetCharacterStoryGoal(CharacterType type) const;
    bool MoveCharacterToward(Character& character, const glm::vec3& target, float dt);
    void TryAdvanceOffscreenStory(Character& character, std::size_t index, float dt, bool atGoal);
    void RenderInteriorZoneBounds(const Camera& camera, float aspectRatio) const;
};
