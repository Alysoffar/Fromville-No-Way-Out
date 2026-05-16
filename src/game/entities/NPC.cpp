#include "game/entities/NPC.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include <glm/gtc/constants.hpp>

NPC::NPC(std::string displayName, glm::vec3 home)
    : Entity(std::move(displayName)), homePosition(home) {
    transform.position = homePosition;
    BuildRoute();
}

void NPC::Update(float dt) {
    routineTimer -= dt;

    if (threatVisible) {
        fear = std::min(100.0f, fear + dt * 34.0f);
    } else if (nightMode) {
        fear = std::max(18.0f, fear - dt * 6.0f);
    } else {
        fear = std::max(0.0f, fear - dt * 11.0f);
    }

    if (threatVisible && fear >= 62.0f) {
        aiState = NPCAIState::Panic;
        FleeFromThreat(dt);
    } else if (threatVisible) {
        aiState = NPCAIState::Afraid;
        FleeFromThreat(dt);
    } else if (rescueRequested && !nightMode && fear < 58.0f) {
        aiState = NPCAIState::Rescue;
        MoveToward(rescueTarget, dt);
    } else if (nightMode) {
        aiState = NPCAIState::Shelter;
        routeWaitRemaining = 0.0f;
        if (glm::length(transform.position - homePosition) > 0.2f) {
            MoveToward(homePosition, dt);
        }
    } else {
        aiState = NPCAIState::Routine;
        if (routePoints.empty()) {
            BuildRoute();
        }

        if (routeWaitRemaining > 0.0f) {
            routeWaitRemaining -= dt;
        } else {
            const glm::vec3 target = GetCurrentRouteTarget();
            glm::vec3 toTarget = target - transform.position;
            toTarget.y = 0.0f;

            if (glm::length(toTarget) < 0.4f) {
                AdvanceRoute();
                routeWaitRemaining = 0.65f + static_cast<float>(routeIndex % 3) * 0.35f;
            } else if (glm::length(toTarget) > 0.001f) {
                MoveToward(target, dt);
            }
        }
    }

    ApplyPhysics(dt);
}

void NPC::SetNight(bool night) {
    nightMode = night;
    if (nightMode) {
        routeWaitRemaining = 0.0f;
    }
}

void NPC::SetThreatPosition(const glm::vec3& enemyPosition, bool visible) {
    threatPosition = enemyPosition;
    threatVisible = visible;
    if (visible) {
        routeWaitRemaining = 0.0f;
    }
}

void NPC::SetRescueTarget(const glm::vec3& targetPosition, bool shouldRescue) {
    rescueTarget = targetPosition;
    rescueRequested = shouldRescue;
}

bool NPC::IsInDanger() const {
    return threatVisible || aiState == NPCAIState::Panic || fear >= 70.0f;
}

float NPC::GetMoveSpeed() const {
    switch (aiState) {
        case NPCAIState::Panic: return 4.8f;
        case NPCAIState::Afraid: return 3.7f;
        case NPCAIState::Rescue: return 3.0f;
        case NPCAIState::Shelter: return 1.65f;
        case NPCAIState::Routine: return 2.0f;
    }

    return 2.0f;
}

glm::vec3 NPC::GetDebugColor() const {
    switch (aiState) {
        case NPCAIState::Panic: return glm::vec3(1.0f, 0.18f, 0.12f);
        case NPCAIState::Afraid: return glm::vec3(0.98f, 0.82f, 0.2f);
        case NPCAIState::Rescue: return glm::vec3(0.55f, 0.92f, 1.0f);
        case NPCAIState::Shelter: return glm::vec3(0.35f, 0.55f, 0.95f);
        case NPCAIState::Routine: return glm::vec3(0.25f, 0.90f, 0.65f);
    }

    return glm::vec3(0.25f, 0.90f, 0.65f);
}

void NPC::BuildRoute() {
    routePoints.clear();

    const float lateral = 2.4f + static_cast<float>(homePosition.x * 0.1f);
    const float depth = 2.0f + static_cast<float>(homePosition.z * 0.1f);

    routePoints.push_back(homePosition + glm::vec3(-lateral, 0.0f, 0.8f));
    routePoints.push_back(homePosition + glm::vec3(-0.8f, 0.0f, depth));
    routePoints.push_back(homePosition + glm::vec3(lateral, 0.0f, 0.4f));
    routePoints.push_back(homePosition + glm::vec3(0.6f, 0.0f, -depth));
    routePoints.push_back(homePosition + glm::vec3(-1.2f, 0.0f, -0.5f));

    routeIndex = 0;
    routineTimer = 0.0f;
    routeWaitRemaining = 0.35f;
}

void NPC::AdvanceRoute() {
    if (routePoints.empty()) {
        return;
    }

    routeIndex = (routeIndex + 1) % routePoints.size();
    routineTimer = 1.25f;
}

glm::vec3 NPC::GetCurrentRouteTarget() const {
    if (routePoints.empty()) {
        return homePosition;
    }

    return routePoints[routeIndex % routePoints.size()];
}

void NPC::MoveToward(const glm::vec3& target, float dt) {
    glm::vec3 direction = target - transform.position;
    direction.y = 0.0f;
    if (glm::length(direction) <= 0.001f) {
        return;
    }

    direction = glm::normalize(direction);
    Move(direction.x, direction.z, dt);
    transform.rotation.y = glm::degrees(std::atan2(direction.x, direction.z));
}

void NPC::FleeFromThreat(float dt) {
    glm::vec3 fleeDirection = transform.position - threatPosition;
    fleeDirection.y = 0.0f;
    if (glm::length(fleeDirection) <= 0.001f) {
        fleeDirection = homePosition - transform.position;
        fleeDirection.y = 0.0f;
    }

    if (glm::length(fleeDirection) <= 0.001f) {
        return;
    }

    fleeDirection = glm::normalize(fleeDirection);
    Move(fleeDirection.x, fleeDirection.z, dt);
    transform.rotation.y = glm::degrees(std::atan2(fleeDirection.x, fleeDirection.z));
}
