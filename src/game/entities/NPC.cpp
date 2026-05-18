#include "game/entities/NPC.h"
#include "engine/physics/CollisionWorld.h"
#include "engine/renderer/Animation.h"
#include "engine/renderer/Animator.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <cctype>

static glm::vec3 GetPrimaryWorkplace(const std::string& name) {
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c){ return std::tolower(c); });
    
    if (lower == "mara" || lower == "elena" || lower == "tom") {
        return glm::vec3(-35.0f, 2.0f, -35.0f); // Diner
    } else if (lower == "boyd" || lower == "kenny") {
        return glm::vec3(-9.0f, 2.0f, 8.0f); // Sheriff Station
    } else if (lower == "kristi" || lower == "sara") {
        return glm::vec3(-10.0f, 2.0f, -2.0f); // Church/Clinic
    } else if (lower == "ellis" || lower == "fatima" || lower == "donna") {
        return glm::vec3(9.0f, 2.0f, 8.0f); // Colony House
    } else if (lower == "victor") {
        return glm::vec3(9.0f, 2.0f, 8.0f); // Colony House
    } else if (lower == "jade") {
        return glm::vec3(-11.0f, 2.0f, -1.0f); // Victor's Hideout
    } else if (lower == "tabitha") {
        return glm::vec3(-9.0f, 2.0f, 8.0f); // House near Sheriff Station
    }
    return glm::vec3(-35.0f, 2.0f, -35.0f); // Default to Diner
}

NPC::NPC(std::string displayName, glm::vec3 home)
    : Entity(std::move(displayName)), homePosition(home) {
    transform.position = homePosition;
    BuildRoute();
}

void NPC::LoadDeferredMesh() {
    std::string lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), [](unsigned char c){ return std::tolower(c); });
    
    // Map missing NPC assets to existing character/NPC folders in assets/models/characters/
    std::string modelName = lowerName;
    if (lowerName == "kenny") {
        modelName = "tom";
    } else if (lowerName == "kristi") {
        modelName = "mara";
    } else if (lowerName == "ellis") {
        modelName = "jade";
    } else if (lowerName == "fatima") {
        modelName = "elena";
    } else if (lowerName == "donna") {
        modelName = "tabitha";
    }

    LoadMesh("assets/models/characters/" + modelName + "/" + modelName + ".fbx",
             "assets/models/characters/" + modelName + "/" + modelName + "_Walking.fbx");
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

        // State update using routineTimer as countdown and routeIndex as state selector:
        // - routeIndex % 2 == 0: WORK state. Stay near workplace.
        // - routeIndex % 2 == 1: CHORE state. Travel to another POI.

        if (routineTimer <= 0.0f) {
            // Switch state!
            routeIndex++;
            if (routeIndex % 2 == 0) {
                // Switch to WORK: stay at workplace for 25 to 40 seconds
                routineTimer = 25.0f + static_cast<float>(rand() % 15);
                // Choose a random spot near workplace
                glm::vec3 workplace = GetPrimaryWorkplace(name);
                float angle = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 2.0f * 3.14159f;
                float dist = 2.0f + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 4.5f;
                targetGoal = workplace + glm::vec3(std::cos(angle) * dist, 0.0f, std::sin(angle) * dist);
            } else {
                // Switch to CHORE: choose a random other POI to visit for 15 to 25 seconds
                routineTimer = 15.0f + static_cast<float>(rand() % 10);
                if (!poiPoints.empty()) {
                    glm::vec3 workplace = GetPrimaryWorkplace(name);
                    std::vector<glm::vec3> validPois;
                    for (const auto& p : poiPoints) {
                        if (glm::distance(p, workplace) > 5.0f) {
                            validPois.push_back(p);
                        }
                    }
                    if (!validPois.empty()) {
                        targetGoal = validPois[rand() % validPois.size()];
                    } else {
                        targetGoal = poiPoints[rand() % poiPoints.size()];
                    }
                } else {
                    targetGoal = GetPrimaryWorkplace(name);
                }
            }
            routeWaitRemaining = 0.5f + static_cast<float>(rand() % 3) * 0.4f;
            wantsToMove = false;
        } else {
            routineTimer -= dt;
        }

        if (routeWaitRemaining > 0.0f) {
            routeWaitRemaining -= dt;
            wantsToMove = false;
        } else {
            float distToGoal = glm::distance(transform.position, targetGoal);
            if (distToGoal < 0.55f) {
                if (routeIndex % 2 == 0) {
                    // Pick another spot near workplace to simulate active tasks!
                    glm::vec3 workplace = GetPrimaryWorkplace(name);
                    float angle = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 2.0f * 3.14159f;
                    float dist = 2.0f + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 4.5f;
                    targetGoal = workplace + glm::vec3(std::cos(angle) * dist, 0.0f, std::sin(angle) * dist);
                    routeWaitRemaining = 1.0f + static_cast<float>(rand() % 3) * 0.5f;
                } else {
                    // Chore completed! Set routineTimer to 0 to trigger WORK state switch
                    routineTimer = 0.0f;
                }
                wantsToMove = false;
            } else {
                wantsToMove = true;
            }
        }
    }

    glm::vec3 startPos = transform.position;

    if (wantsToMove) {
        glm::vec3 direction = targetGoal - transform.position;
        direction.y = 0.0f;
        if (glm::length(direction) > 0.001f) {
            direction = glm::normalize(direction);
        }

        // Stuck mitigation logic
        if (wanderTimer > 0.0f) {
            wanderTimer -= dt;

            // Wandering direction XZ plane
            glm::vec3 wanderDir(std::sin(wanderAngle), 0.0f, std::cos(wanderAngle));
            if (glm::length(wanderDir) > 0.001f) {
                wanderDir = glm::normalize(wanderDir);
            }

            Move(wanderDir.x, wanderDir.z, dt);
            if (glm::length(wanderDir) > 0.001f) {
                float targetAngle = glm::degrees(std::atan2(wanderDir.x, wanderDir.z));
                float currentAngle = transform.rotation.y;
                float diff = targetAngle - currentAngle;
                while (diff < -180.0f) diff += 360.0f;
                while (diff > 180.0f) diff -= 360.0f;
                transform.rotation.y = currentAngle + diff * glm::clamp(8.0f * dt, 0.0f, 1.0f);
            }
            SetCurrentSpeed(GetMoveSpeed() * 0.6f);
        } else {
            Move(direction.x, direction.z, dt);
            if (glm::length(direction) > 0.001f) {
                float targetAngle = glm::degrees(std::atan2(direction.x, direction.z));
                float currentAngle = transform.rotation.y;
                float diff = targetAngle - currentAngle;
                while (diff < -180.0f) diff += 360.0f;
                while (diff > 180.0f) diff -= 360.0f;
                transform.rotation.y = currentAngle + diff * glm::clamp(8.0f * dt, 0.0f, 1.0f);
            }
            SetCurrentSpeed(GetMoveSpeed() * 0.6f);
        }

        // Stuck detection
        float distMoved = glm::length(glm::vec3(transform.position.x - startPos.x, 0.0f, transform.position.z - startPos.z));
        if (distMoved < dt * 0.1f) {
            stuckTimer += dt;
            if (stuckTimer > stuckThreshold) {
                // Determine a bypass angle 90 degrees to the left or right of our desired direction
                float desiredAngle = std::atan2(direction.x, direction.z);
                float sideSign = ((rand() % 2) == 0) ? 1.0f : -1.0f;
                wanderAngle = desiredAngle + sideSign * (3.14159f * 0.5f); // 90 degrees offset
                wanderTimer = 0.50f; // Wander sideways for 0.50 seconds to clear the obstacle
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

    if (wantsToMove && m_FramePosHistory.size() == 12 && glm::distance(m_FramePosHistory.front(), transform.position) < 0.02f) {
        glm::vec3 testPos = transform.position + glm::vec3(0.0f, 0.4f, 0.0f);
        if (collisionWorld && !collisionWorld->IsPositionOccupied(testPos, 0.5f)) {
            transform.position = testPos;
            m_FramePosHistory.clear();
            
            // Choose a new target position or destination
            if (aiState == NPCAIState::Routine) {
                AdvanceRoute();
            } else {
                wanderAngle = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f * glm::pi<float>();
                wanderTimer = wanderChangeInterval;
            }
        }
    }
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
        wanderAngle = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f * glm::pi<float>();
    } else {
        aiState = NPCAIState::Routine;
        routeWaitRemaining = 0.0f;
        stuckTimer = 0.0f;
        wanderTimer = 0.0f;
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
    routePoints.push_back(GetPrimaryWorkplace(name));
    
    routeIndex = 0;
    routineTimer = 0.0f; // Force initial state switch
    routeWaitRemaining = 0.5f;
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