#include "game/entities/Enemy.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float kAttackRange = 1.15f;
constexpr float kAggroRange = 22.0f;
constexpr float kInvestigateThreshold = 0.18f;
constexpr float kChaseThreshold = 0.68f;
}

Enemy::Enemy(glm::vec3 spawnPosition)
    : Entity("Monster"), spawnPosition(spawnPosition), lastKnownTargetPosition(spawnPosition) {
    transform.position = spawnPosition;
    BuildPatrolRoute();

    // Dynamically assign enemy1 or enemy2 based on spawn coordinates for great visual variety
    std::string enemyDir = (static_cast<int>(std::abs(spawnPosition.x) + std::abs(spawnPosition.z)) % 2 == 0) ? "enemy1" : "enemy2";
    LoadMesh("assets/models/characters/" + enemyDir + "/" + enemyDir + ".fbx",
             "assets/models/characters/" + enemyDir + "/" + enemyDir + "_walk.fbx");
}

void Enemy::Update(float dt) {
    if (!nightMode) {
        // Dormant during the day
        return;
    }
    if (playerKilled) {
        ApplyPhysics(dt);
        return;
    }

    wanderTimer += dt;

    const float stimulus = std::clamp(
        perception.proximity * 0.45f + perception.sound * 0.35f + perception.light * 0.20f,
        0.0f,
        1.0f);

    if (perception.targetSheltered) {
        // Targets inside shelters: approach the shelter edge and torment
        aiState = EnemyAIState::Torment;
        lastKnownTargetPosition = perception.targetPosition;
        // Move toward the shelter edge but do not enter
        glm::vec3 toTarget = lastKnownTargetPosition - transform.position;
        toTarget.y = 0.0f;
        const float dist = glm::length(toTarget);
        if (dist > 1.0f) {
            MoveToward(lastKnownTargetPosition, dt, 0.06f);
        }
    } else if (perception.visible || stimulus > kInvestigateThreshold) {
        lastKnownTargetPosition = perception.targetPosition;
        investigationTimer = 5.0f + stimulus * 4.0f;
        alertness = std::min(1.0f, alertness + dt * (0.45f + stimulus));
    } else {
        alertness = std::max(0.0f, alertness - dt * 0.12f);
        investigationTimer = std::max(0.0f, investigationTimer - dt);
    }

    glm::vec3 toTarget = lastKnownTargetPosition - transform.position;
    toTarget.y = 0.0f;
    const float targetDistance = glm::length(toTarget);

    if (perception.visible && targetDistance <= kAttackRange) {
        aiState = EnemyAIState::Attack;
        if (perception.playerTarget) {
            playerKilled = true;
        }
    } else if (perception.visible && (stimulus >= kChaseThreshold || targetDistance <= 5.0f)) {
        aiState = EnemyAIState::Chase;
        MoveToward(lastKnownTargetPosition, dt, 0.24f);
    } else if (perception.visible) {
        aiState = EnemyAIState::Stalk;
        MoveToward(lastKnownTargetPosition, dt, 0.08f);
    } else if (investigationTimer > 0.0f) {
        aiState = EnemyAIState::Investigate;
        if (targetDistance > 0.45f) {
            MoveToward(lastKnownTargetPosition, dt, 0.16f);
        }
    } else {
        aiState = EnemyAIState::Patrol;
        const glm::vec3 patrolTarget = patrolPoints[patrolIndex];
        glm::vec3 toPatrol = patrolTarget - transform.position;
        toPatrol.y = 0.0f;
        if (glm::length(toPatrol) < 0.55f) {
            patrolIndex = (patrolIndex + 1) % 4;
        } else {
            MoveToward(patrolTarget, dt, 0.05f);
        }
    }

    ApplyPhysics(dt);
}

void Enemy::SetNight(bool night) {
    nightMode = night;
    if (!nightMode) {
        // Reset to spawn and idle during day
        aiState = EnemyAIState::Patrol;
        lastKnownTargetPosition = spawnPosition;
        targetPosition = spawnPosition;
        perception = EnemyPerception();
    }
}

void Enemy::SetTarget(const glm::vec3& target) {
    targetPosition = target;
}

void Enemy::SetVisibleTarget(const glm::vec3& target, bool visible) {
    targetPosition = target;
    targetVisible = visible;

    EnemyPerception legacyPerception;
    legacyPerception.targetPosition = target;
    legacyPerception.hasTarget = true;
    legacyPerception.playerTarget = true;
    legacyPerception.visible = visible;
    legacyPerception.proximity = visible ? 1.0f : 0.0f;
    SetPerception(legacyPerception);
}

void Enemy::SetPerception(const EnemyPerception& newPerception) {
    perception = newPerception;
    targetPosition = newPerception.targetPosition;
    targetVisible = newPerception.visible;
}

bool Enemy::HasKilledPlayer() const {
    return playerKilled;
}

float Enemy::GetMoveSpeed() const {
    if (playerKilled) {
        return 0.0f;
    }

    switch (aiState) {
        case EnemyAIState::Patrol: return 1.35f;
        case EnemyAIState::Investigate: return 1.95f;
        case EnemyAIState::Stalk: return 1.45f;
        case EnemyAIState::Chase: return 3.15f;
        case EnemyAIState::Torment: return 0.9f;
        case EnemyAIState::Attack: return 0.0f;
    }

    return 2.15f;
}

glm::vec3 Enemy::GetDebugColor() const {
    if (playerKilled) {
        return glm::vec3(0.35f);
    }

    switch (aiState) {
        case EnemyAIState::Patrol: return glm::vec3(0.55f, 0.95f, 0.42f);
        case EnemyAIState::Investigate: return glm::vec3(0.95f, 0.78f, 0.26f);
        case EnemyAIState::Stalk: return glm::vec3(0.95f, 0.42f, 0.18f);
        case EnemyAIState::Chase: return glm::vec3(1.0f, 0.08f, 0.05f);
        case EnemyAIState::Torment: return glm::vec3(0.6f, 0.1f, 0.7f);
        case EnemyAIState::Attack: return glm::vec3(0.35f);
    }

    return glm::vec3(0.95f, 0.22f, 0.18f);
}

void Enemy::BuildPatrolRoute() {
    patrolPoints[0] = spawnPosition + glm::vec3(-4.0f, 0.0f, -2.0f);
    patrolPoints[1] = spawnPosition + glm::vec3(2.5f, 0.0f, -4.0f);
    patrolPoints[2] = spawnPosition + glm::vec3(4.0f, 0.0f, 2.5f);
    patrolPoints[3] = spawnPosition + glm::vec3(-2.5f, 0.0f, 3.5f);
}

void Enemy::MoveToward(const glm::vec3& target, float dt, float swayAmount) {
    glm::vec3 direction = target - transform.position;
    direction.y = 0.0f;
    if (glm::length(direction) <= 0.001f) {
        return;
    }

    direction = glm::normalize(direction);
    const float sway = std::sin(wanderTimer * 2.2f) * swayAmount;
    direction += glm::vec3(-direction.z, 0.0f, direction.x) * sway;
    if (glm::length(direction) <= 0.001f) {
        return;
    }

    direction = glm::normalize(direction);
    Move(direction.x, direction.z, dt);
    transform.rotation.y = glm::degrees(std::atan2(direction.x, direction.z));
}

bool Enemy::IsInAttackRange(const glm::vec3& targetPos) const {
    glm::vec3 delta = targetPos - transform.position;
    delta.y = 0.0f;
    return glm::length(delta) <= kAttackRange;
}