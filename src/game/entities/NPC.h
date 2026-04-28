#pragma once

#include <vector>

#include "game/entities/Entity.h"

class NPC : public Entity {
public:
    explicit NPC(std::string displayName = "NPC", glm::vec3 homePosition = glm::vec3(0.0f));
    virtual ~NPC() = default;

    void Update(float dt) override;
    void SetNight(bool night);
    void SetThreatPosition(const glm::vec3& enemyPosition, bool threatVisible);

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
    glm::vec3 threatPosition = glm::vec3(0.0f);

    void BuildRoute();
    void AdvanceRoute();
    glm::vec3 GetCurrentRouteTarget() const;
};