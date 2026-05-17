#pragma once

#include <vector>

#include "game/entities/Entity.h"

enum class NPCAIState {
    Routine,
    Conversing,
    Shelter,
    Afraid,
    Panic,
    Rescue
};

class NPC : public Entity {
public:
    explicit NPC(std::string displayName = "NPC", glm::vec3 homePosition = glm::vec3(0.0f));
    virtual ~NPC() = default;

    void Update(float dt) override;
    void SetNight(bool night);
    void SetPOIs(const std::vector<glm::vec3>& pois);
    void StartConversation(float seconds);
    void SetThreatPosition(const glm::vec3& enemyPosition, bool threatVisible);
    void SetRescueTarget(const glm::vec3& targetPosition, bool shouldRescue);
    bool IsInDanger() const;
    float GetFear() const { return fear; }
    NPCAIState GetAIState() const { return aiState; }

    glm::vec3 GetDebugColor() const override;

protected:
    float GetMoveSpeed() const override;

private:
    glm::vec3 homePosition = glm::vec3(0.0f);
    std::vector<glm::vec3> routePoints;
    std::size_t routeIndex = 0;
    float routineTimer = 0.0f;
    float routeWaitRemaining = 0.0f;
    bool nightMode = false;
    bool threatVisible = false;
    bool rescueRequested = false;
    float fear = 0.0f;
    NPCAIState aiState = NPCAIState::Routine;
    glm::vec3 threatPosition = glm::vec3(0.0f);
    glm::vec3 rescueTarget = glm::vec3(0.0f);
    std::vector<glm::vec3> poiPoints;
    float conversingTimer = 0.0f;

    void BuildRoute();
    void AdvanceRoute();
    glm::vec3 GetCurrentRouteTarget() const;
    void MoveToward(const glm::vec3& target, float dt);
    void FleeFromThreat(float dt);
};