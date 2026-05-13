#pragma once

#include "game/entities/Player.h"
#include "engine/physics/CollisionWorld.h"

class Camera;
class DayNightCycle;

class World {
public:
    World();

    void Initialize();
    void Update(const Camera& camera, float dt);
    void Render(const Camera& camera, float aspectRatio);
    void RenderObjects(const Camera& camera, float aspectRatio, const DayNightCycle& dayNight);

    Player& GetPlayer() { return player; }

private:
    Player player;
    CollisionWorld collisionWorld;
};