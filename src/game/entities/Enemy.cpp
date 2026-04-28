#include "game/entities/Enemy.h"

#include <cmath>

namespace {
constexpr float kAttackRange = 1.15f;
constexpr float kAggroRange = 22.0f;
}

Enemy::Enemy(glm::vec3 spawnPosition)
    : Entity("Enemy") {
    transform.position = spawnPosition;
}

void Enemy::Update(float dt) {
    if (!playerKilled) {
        wanderTimer += dt;

        glm::vec3 toTarget = targetPosition - transform.position;
        toTarget.y = 0.0f;

        const float distance = glm::length(toTarget);
        if (targetVisible && distance <= kAttackRange) {
            playerKilled = true;
        } else if (targetVisible && distance <= kAggroRange && distance > 0.001f) {
            glm::vec3 chaseDirection = glm::normalize(toTarget);
            const float sway = std::sin(wanderTimer * 2.2f) * 0.22f;
            chaseDirection += glm::vec3(-chaseDirection.z, 0.0f, chaseDirection.x) * sway;
            if (glm::length(chaseDirection) > 0.001f) {
                chaseDirection = glm::normalize(chaseDirection);
                Move(chaseDirection.x, chaseDirection.z, dt);
            }

            transform.rotation.y = glm::degrees(std::atan2(chaseDirection.x, chaseDirection.z));
        }
    }

    ApplyPhysics(dt);
}

void Enemy::SetTarget(const glm::vec3& target) {
    targetPosition = target;
}

void Enemy::SetVisibleTarget(const glm::vec3& target, bool visible) {
    targetPosition = target;
    targetVisible = visible;
}

bool Enemy::HasKilledPlayer() const {
    return playerKilled;
}

float Enemy::GetMoveSpeed() const {
    return playerKilled ? 0.0f : 2.15f;
}

glm::vec3 Enemy::GetDebugColor() const {
    return playerKilled ? glm::vec3(0.35f) : glm::vec3(0.95f, 0.22f, 0.18f);
}