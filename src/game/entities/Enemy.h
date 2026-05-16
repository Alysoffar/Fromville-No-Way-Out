#pragma once

#include <glm/glm.hpp>

#include "game/entities/Entity.h"

struct EnemyPerception {
    glm::vec3 targetPosition = glm::vec3(0.0f);
    bool hasTarget = false;
    bool playerTarget = false;
    bool visible = false;
    float sound = 0.0f;
    float light = 0.0f;
    float proximity = 0.0f;
};

enum class EnemyAIState {
    Patrol,
    Investigate,
    Stalk,
    Chase,
    Attack
};

class Enemy : public Entity {
public:
    explicit Enemy(glm::vec3 spawnPosition = glm::vec3(0.0f));

    void Update(float dt) override;

    void SetTarget(const glm::vec3& targetPosition);
    void SetVisibleTarget(const glm::vec3& targetPosition, bool targetVisible);
    void SetPerception(const EnemyPerception& newPerception);
    bool HasKilledPlayer() const;
    bool IsInAttackRange(const glm::vec3& targetPos) const;
    EnemyAIState GetAIState() const { return aiState; }

    glm::vec3 GetDebugColor() const override;

protected:
    float GetMoveSpeed() const override;

private:
    glm::vec3 spawnPosition = glm::vec3(0.0f);
    glm::vec3 targetPosition = glm::vec3(0.0f);
    glm::vec3 lastKnownTargetPosition = glm::vec3(0.0f);
    glm::vec3 patrolPoints[4] = {};
    int patrolIndex = 0;
    bool targetVisible = false;
    bool playerKilled = false;
    float wanderTimer = 0.0f;
    float alertness = 0.0f;
    float investigationTimer = 0.0f;
    EnemyPerception perception;
    EnemyAIState aiState = EnemyAIState::Patrol;

    void BuildPatrolRoute();
    void MoveToward(const glm::vec3& target, float dt, float swayAmount);
};
