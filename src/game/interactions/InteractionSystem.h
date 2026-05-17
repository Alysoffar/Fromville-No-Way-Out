#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>

class Character;
class QuestSystem;
enum class CharacterType;

enum class InteractionType {
    Conversation,
    ItemPickup,
    Door,
    Trigger
};

struct InteractionNode {
    std::string id;
    InteractionType type = InteractionType::Trigger;
    std::string name;
    glm::vec3 position = glm::vec3(0.0f);
    float radius = 2.0f;
    bool active = true;
    bool open = false;
    bool collected = false;
    std::string prompt;
    std::string successMessage;
    std::string questFlag;
    std::string requiredFlag;
    int questObjectiveIndex = -1;
    int questSubObjectiveIndex = -1;
    std::string requiredCharacter;
    bool followsActiveCharacter = false;
    glm::vec3 originalPosition = glm::vec3(0.0f);
};

class InteractionSystem {
public:
    void Initialize();

    std::string GetPromptFor(const Character& character, const QuestSystem& questSystem, bool hasActiveQuest, CharacterType activeQuestCharacter) const;
    bool TryInteract(Character& character, QuestSystem& questSystem, bool hasActiveQuest, CharacterType activeQuestCharacter);
    // Try pickup-only interaction (used for item pickup bound to a separate key)
    bool TryPickup(Character& character, QuestSystem& questSystem, bool hasActiveQuest, CharacterType activeQuestCharacter);
    void MarkQuestStepSolved(CharacterType characterType, int objectiveIndex);
    const std::string& GetLastInteractionMessage() const { return lastInteractionMessage; }
    const std::vector<InteractionNode>& GetNodes() const { return nodes; }
    std::vector<InteractionNode>& GetNodes() { return nodes; }
    bool HasLastQuestObjective() const { return lastInteractionQuestObjectiveIndex >= 0; }
    CharacterType GetLastInteractionQuestCharacter() const { return lastInteractionQuestCharacter; }
    int GetLastInteractionQuestObjectiveIndex() const { return lastInteractionQuestObjectiveIndex; }
    int GetLastInteractionQuestSubObjectiveIndex() const { return lastInteractionQuestSubObjectiveIndex; }

    void Update(float dt, QuestSystem& questSystem, const glm::vec3& activeCharPos = glm::vec3(0.0f), float activeCharRotY = 0.0f);
    // Add a new interaction node at runtime (used for spawning checkpoints)
    void AddNode(const InteractionNode& node);

private:
    std::vector<InteractionNode> nodes;
    std::string lastInteractionMessage;
    CharacterType lastInteractionQuestCharacter{};
    int lastInteractionQuestObjectiveIndex = -1;
    int lastInteractionQuestSubObjectiveIndex = -1;

    bool LoadFromConfig(const std::string& path);
    void LoadFallbackNodes();

    const InteractionNode* FindBestNode(const Character& character, const QuestSystem& questSystem) const;
    InteractionNode* FindBestNode(const Character& character, const QuestSystem& questSystem);
};
