#pragma once

class Camera;
class MapManager;
class TerrainRenderer;

class World {
public:
    World();

    void Initialize();
    void Update(const Camera& camera, float dt);
    void Render(const Camera& camera, float aspectRatio);

private:
    MapManager* mapManager = nullptr;
    TerrainRenderer* terrain = nullptr;
};