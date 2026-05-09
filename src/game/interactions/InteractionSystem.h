#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>

class Character;
class QuestSystem;

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

    std::string GetPromptFor(const Character& character, const QuestSystem& questSystem) const;
    bool TryInteract(Character& character, QuestSystem& questSystem);
    const std::string& GetLastInteractionMessage() const { return lastInteractionMessage; }

    void Update(float dt, QuestSystem& questSystem);

private:
    std::vector<InteractionNode> nodes;
    std::string lastInteractionMessage;

    bool LoadFromConfig(const std::string& path);
    void LoadFallbackNodes();

    const InteractionNode* FindBestNode(const Character& character, const QuestSystem& questSystem) const;
    InteractionNode* FindBestNode(const Character& character, const QuestSystem& questSystem);
};
