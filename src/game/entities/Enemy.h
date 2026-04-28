#pragma once

#include <glm/glm.hpp>

#include "game/entities/Entity.h"

class Enemy : public Entity {
public:
    explicit Enemy(glm::vec3 spawnPosition = glm::vec3(0.0f));

    void Update(float dt) override;

    void SetTarget(const glm::vec3& targetPosition);
    void SetVisibleTarget(const glm::vec3& targetPosition, bool targetVisible);
    bool HasKilledPlayer() const;

    glm::vec3 GetDebugColor() const override;

protected:
    float GetMoveSpeed() const override;

private:
    glm::vec3 targetPosition = glm::vec3(0.0f);
    bool targetVisible = false;
    bool playerKilled = false;
    float wanderTimer = 0.0f;
};