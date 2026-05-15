#pragma once

#include <vector>
#include "game/entities/Player.h"
#include "game/entities/Door.h"
#include "engine/physics/CollisionWorld.h"

class Camera;
class DayNightCycle;

class World {
public:
    World();

    void Initialize();
    void Update(const Camera& camera, float dt);
    void Render(const Camera& camera, float aspectRatio);
    void RenderObjects(const Camera& camera, float aspectRatio, const DayNightCycle& dayNight, float fogDensity);
    void TryInteract();
    void TryExit();

    Player& GetPlayer() { return player; }
    CollisionWorld* GetCollisionWorld() { return &collisionWorld; }
    bool IsInsideBuilding() const { return isInsideBuilding; }

private:
    Player player;
    CollisionWorld collisionWorld;
    std::vector<Door> doors;
    
    bool isInsideBuilding = false;
    glm::vec3 previousOutsidePosition;
};