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
    std::string requiredCharacter;
};

class InteractionSystem {
public:
    void Initialize();

    std::string GetPromptFor(const Character& character, const QuestSystem& questSystem, bool hasActiveQuest, CharacterType activeQuestCharacter) const;
    bool TryInteract(Character& character, QuestSystem& questSystem, bool hasActiveQuest, CharacterType activeQuestCharacter);
    const std::string& GetLastInteractionMessage() const { return lastInteractionMessage; }
    const std::vector<InteractionNode>& GetNodes() const { return nodes; }
    bool HasLastQuestObjective() const { return lastInteractionQuestObjectiveIndex >= 0; }
    CharacterType GetLastInteractionQuestCharacter() const { return lastInteractionQuestCharacter; }
    int GetLastInteractionQuestObjectiveIndex() const { return lastInteractionQuestObjectiveIndex; }

    void Update(float dt, QuestSystem& questSystem);

private:
    std::vector<InteractionNode> nodes;
    std::string lastInteractionMessage;
    CharacterType lastInteractionQuestCharacter{};
    int lastInteractionQuestObjectiveIndex = -1;

    bool LoadFromConfig(const std::string& path);
    void LoadFallbackNodes();

    const InteractionNode* FindBestNode(const Character& character, const QuestSystem& questSystem) const;
    InteractionNode* FindBestNode(const Character& character, const QuestSystem& questSystem);
};
