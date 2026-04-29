#pragma once

#include "game/entities/Player.h"

class Camera;
class MapManager;
class TerrainRenderer;

class World {
public:
    World();

    void Initialize();
    void Update(const Camera& camera, float dt);
    void Render(const Camera& camera, float aspectRatio);

    Player& GetPlayer() { return player; }

private:
    MapManager* mapManager = nullptr;
    TerrainRenderer* terrain = nullptr;
    Player player;
};