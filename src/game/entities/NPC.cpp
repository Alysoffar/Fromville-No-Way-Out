#include "game/entities/NPC.h"

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
        glm::vec3 fleeDirection = transform.position - threatPosition;
        fleeDirection.y = 0.0f;

        if (glm::length(fleeDirection) > 0.001f) {
            fleeDirection = glm::normalize(fleeDirection);
            Move(fleeDirection.x, fleeDirection.z, dt);
            transform.rotation.y = glm::degrees(std::atan2(fleeDirection.x, fleeDirection.z));
        }
    } else if (nightMode) {
        routeWaitRemaining = 0.0f;
        if (glm::length(transform.position - homePosition) > 0.2f) {
            glm::vec3 toHome = homePosition - transform.position;
            toHome.y = 0.0f;
            if (glm::length(toHome) > 0.001f) {
                toHome = glm::normalize(toHome);
                Move(toHome.x, toHome.z, dt);
            }
        }
    } else {
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
                toTarget = glm::normalize(toTarget);
                Move(toTarget.x, toTarget.z, dt);
                transform.rotation.y = glm::degrees(std::atan2(toTarget.x, toTarget.z));
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

float NPC::GetMoveSpeed() const {
    if (threatVisible) {
        return 4.6f;
    }

    return nightMode ? 1.4f : 2.0f;
}

glm::vec3 NPC::GetDebugColor() const {
    if (threatVisible) {
        return glm::vec3(0.98f, 0.82f, 0.2f);
    }

    return nightMode ? glm::vec3(0.35f, 0.55f, 0.95f) : glm::vec3(0.25f, 0.90f, 0.65f);
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