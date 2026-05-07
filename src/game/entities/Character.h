#pragma once

#include "game/entities/Entity.h"
#include <glm/glm.hpp>

enum class CharacterType {
    Boyd,
    Jade,
    Tabitha,
    Victor,
    Sara
};

class Quest;  // Forward declaration

class Character : public Entity {
public:
    explicit Character(CharacterType type, std::string name, glm::vec3 startPos = glm::vec3(0.0f));
    virtual ~Character() = default;

    void Update(float dt) override;
    virtual void OnSwitchedTo();   // Called when this character becomes active
    virtual void OnSwitchedFrom(); // Called when this character is no longer active
    virtual void ActivateAbility();  // Q-key ability, overridden per character

    CharacterType GetType() const { return type; }
    bool IsActive() const { return isActive; }
    void SetActive(bool active) { isActive = active; }
    
    // Persistent stats
    float GetHealth() const { return health; }
    void SetHealth(float h) { health = glm::clamp(h, 0.0f, 100.0f); }
    void TakeDamage(float dmg) { SetHealth(health - dmg); }
    
    // Quest system
    void SetQuest(Quest* q) { quest = q; }
    Quest* GetQuest() const { return quest; }
    virtual void OnQuestObjectiveComplete(int objectiveIndex) {
        // Override in subclasses to handle quest progression
        (void)objectiveIndex;
    }

protected:
    CharacterType type;
    bool isActive = false;
    float health = 100.0f;
    float abilityTimer = 0.0f;  // Cooldown tracker for abilities
    Quest* quest = nullptr;     // Character's main quest
};
