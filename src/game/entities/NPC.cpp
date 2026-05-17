#include "game/entities/NPC.h"
#include "engine/renderer/Animation.h"
#include "engine/renderer/Animator.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <cctype>

NPC::NPC(std::string displayName, glm::vec3 home)
    : Entity(std::move(displayName)), homePosition(home) {
    transform.position = homePosition;
    BuildRoute();

    std::string lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), [](unsigned char c){ return std::tolower(c); });
    LoadMesh("assets/models/characters/" + lowerName + "/" + lowerName + ".fbx",
             "assets/models/characters/" + lowerName + "/" + lowerName + "_Walking.fbx");
}

void NPC::Update(float dt) {
    routineTimer -= dt;

    if (conversingTimer > 0.0f) {
        conversingTimer = std::max(0.0f, conversingTimer - dt);
        aiState = NPCAIState::Conversing;
        // minimal movement while conversing
        routeWaitRemaining = 0.0f;
        ApplyPhysics(dt);
        stuckTimer = 0.0f;
        SetCurrentSpeed(0.0f);
        lastPosition = transform.position;
        return;
    }

    if (threatVisible) {
        fear = std::min(100.0f, fear + dt * 34.0f);
    } else if (nightMode) {
        fear = std::max(18.0f, fear - dt * 6.0f);
    } else {
        fear = std::max(0.0f, fear - dt * 11.0f);
    }

    glm::vec3 targetGoal = transform.position;
    bool wantsToMove = false;

    if (threatVisible && fear >= 62.0f) {
        aiState = NPCAIState::Panic;
        glm::vec3 fleeDirection = transform.position - threatPosition;
        fleeDirection.y = 0.0f;
        if (glm::length(fleeDirection) <= 0.001f) {
            fleeDirection = homePosition - transform.position;
            fleeDirection.y = 0.0f;
        }
        if (glm::length(fleeDirection) > 0.001f) {
            targetGoal = transform.position + glm::normalize(fleeDirection) * 5.0f;
            wantsToMove = true;
        }
    } else if (threatVisible) {
        aiState = NPCAIState::Afraid;
        glm::vec3 fleeDirection = transform.position - threatPosition;
        fleeDirection.y = 0.0f;
        if (glm::length(fleeDirection) <= 0.001f) {
            fleeDirection = homePosition - transform.position;
            fleeDirection.y = 0.0f;
        }
        if (glm::length(fleeDirection) > 0.001f) {
            targetGoal = transform.position + glm::normalize(fleeDirection) * 5.0f;
            wantsToMove = true;
        }
    } else if (rescueRequested && !nightMode && fear < 58.0f) {
        aiState = NPCAIState::Rescue;
        targetGoal = rescueTarget;
        wantsToMove = (glm::length(targetGoal - transform.position) > 0.4f);
    } else if (nightMode) {
        aiState = NPCAIState::Shelter;
        routeWaitRemaining = 0.0f;
        targetGoal = homePosition;
        wantsToMove = (glm::length(targetGoal - transform.position) > 0.2f);
    } else {
        aiState = NPCAIState::Routine;
        if (routePoints.empty()) {
            BuildRoute();
        }

        if (routeWaitRemaining > 0.0f) {
            routeWaitRemaining -= dt;
        } else {
            targetGoal = GetCurrentRouteTarget();
            glm::vec3 toTarget = targetGoal - transform.position;
            toTarget.y = 0.0f;

            if (glm::length(toTarget) < 0.4f) {
                AdvanceRoute();
                routeWaitRemaining = 0.65f + static_cast<float>(routeIndex % 3) * 0.35f;
            } else if (glm::length(toTarget) > 0.001f) {
                wantsToMove = true;
            }
        }
    }

    glm::vec3 startPos = transform.position;

    if (wantsToMove) {
        // Stuck mitigation logic
        if (wanderTimer > 0.0f) {
            wanderTimer -= dt;

            // Wandering direction XZ plane
            glm::vec3 wanderDir(std::sin(wanderAngle), 0.0f, std::cos(wanderAngle));
            wanderDir = glm::normalize(wanderDir);

            // Slower wander speed (1.2f for NPCs)
            float defaultMoveSpeedScaled = GetMoveSpeed() * 0.6f;
            float scale = 1.2f / defaultMoveSpeedScaled;
            Move(wanderDir.x * scale, wanderDir.z * scale, dt);
            transform.rotation.y = glm::degrees(std::atan2(wanderDir.x, wanderDir.z));
            SetCurrentSpeed(1.2f);
        } else {
            // Standard direct path toward targetGoal
            glm::vec3 direction = targetGoal - transform.position;
            direction.y = 0.0f;
            direction = glm::normalize(direction);

            Move(direction.x, direction.z, dt);
            transform.rotation.y = glm::degrees(std::atan2(direction.x, direction.z));
            SetCurrentSpeed(GetMoveSpeed() * 0.6f);
        }

        // Stuck detection
        float distMoved = glm::length(glm::vec3(transform.position.x - startPos.x, 0.0f, transform.position.z - startPos.z));
        if (distMoved < dt * 0.05f) {
            stuckTimer += dt;
            if (stuckTimer > stuckThreshold) {
                wanderAngle = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f * glm::pi<float>();
                wanderTimer = wanderChangeInterval;
                stuckTimer = 0.0f;
            }
        } else {
            stuckTimer = 0.0f;
        }
    } else {
        stuckTimer = 0.0f;
        SetCurrentSpeed(0.0f);
    }

    lastPosition = transform.position;

    ApplyPhysics(dt);
}

void NPC::UpdateAnimation(float dt) {
    if (!meshLoaded || !animator) return;

    Animation* targetAnim = nullptr;
    if (currentSpeed > 4.0f && runningAnimation && runningAnimation->GetDuration() > 0.0f) {
        targetAnim = runningAnimation.get();
    } else if (currentSpeed > 0.3f && walkingAnimation && walkingAnimation->GetDuration() > 0.0f) {
        targetAnim = walkingAnimation.get();
    } else if (idleAnimation && idleAnimation->GetDuration() > 0.0f) {
        targetAnim = idleAnimation.get();
    }

    if (targetAnim) {
        float animSpeedMultiplier = 1.0f;
        if (targetAnim == runningAnimation.get()) {
            animSpeedMultiplier = currentSpeed / 8.0f;
        } else if (targetAnim == walkingAnimation.get()) {
            animSpeedMultiplier = currentSpeed / 2.0f;
        }
        animSpeedMultiplier = glm::clamp(animSpeedMultiplier, 0.0f, 2.5f);

        animator->SetAnimation(targetAnim);
        animator->UpdateAnimation(dt * animSpeedMultiplier);
    }
}

void NPC::SetPOIs(const std::vector<glm::vec3>& pois) {
    poiPoints = pois;
}

void NPC::StartConversation(float seconds) {
    conversingTimer = seconds;
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
        case NPCAIState::Conversing: return 0.2f;
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
        case NPCAIState::Conversing: return glm::vec3(0.8f, 0.7f, 0.4f);
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

    // If there are town POIs, mix a few into the route for daytime wandering
    if (!poiPoints.empty()) {
        // pick up to 3 nearest POIs
        std::vector<std::pair<float, glm::vec3>> distances;
        for (const auto& p : poiPoints) {
            float d = glm::length(p - homePosition);
            distances.emplace_back(d, p);
        }
        std::sort(distances.begin(), distances.end(), [](const auto& a, const auto& b){ return a.first < b.first; });
        const size_t take = std::min<size_t>(3, distances.size());
        for (size_t i = 0; i < take; ++i) {
            routePoints.push_back(distances[i].second);
        }
    }
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