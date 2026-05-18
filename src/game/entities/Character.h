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

    void Update(float dt) final;
    void UpdateAnimation(float dt) override;
    virtual void OnSwitchedTo();   // Called when this character becomes active
    virtual void OnSwitchedFrom(); // Called when this character is no longer active
    virtual void ActivateAbility();  // Q-key ability, overridden per character
    glm::vec3 GetDebugColor() const override;

    CharacterType GetType() const { return type; }
    bool IsActive() const { return isActive; }
    void SetActive(bool active) { isActive = active; }
    bool IsAbilityReady() const { return abilityTimer <= 0.0f; }
    void StartAbilityCooldown(float seconds) { abilityTimer = seconds; }
    
    // Stuck detection accessors
    float& GetStuckTimer() { return stuckTimer; }
    float GetStuckThreshold() const { return stuckThreshold; }
    glm::vec3& GetLastPosition() { return lastPosition; }
    float& GetWanderAngle() { return wanderAngle; }
    float& GetWanderTimer() { return wanderTimer; }
    float GetWanderChangeInterval() const { return wanderChangeInterval; }
    
    // Persistent stats
    float GetHealth() const { return health; }
    float GetMaxHealth() const { return maxHealth; }
    void SetMaxHealth(float maxH) { maxHealth = maxH; }
    void SetHealth(float h) { health = glm::clamp(h, 0.0f, maxHealth); }
    void TakeDamage(float dmg) { SetHealth(health - dmg); }
    
    // Quest system
    void SetQuest(Quest* q) { quest = q; }
    Quest* GetQuest() const { return quest; }
    virtual void OnQuestObjectiveComplete(int objectiveIndex) {
        // Override in subclasses to handle quest progression
        (void)objectiveIndex;
    }

protected:
    virtual void UpdateCharacterState(float dt);

    CharacterType type;
    bool isActive = false;
    float health = 100.0f;
    float maxHealth = 100.0f;
    float abilityTimer = 0.0f;  // Cooldown tracker for abilities
    Quest* quest = nullptr;     // Character's main quest

    // Stuck and wandering state for offscreen AI
    float stuckTimer = 0.0f;
    float stuckThreshold = 0.15f;
    glm::vec3 lastPosition = glm::vec3(0.0f);
    float wanderAngle = 0.0f;
    float wanderTimer = 0.0f;
    float wanderChangeInterval = 2.5f;
};
