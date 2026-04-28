#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "game/entities/Enemy.h"
#include "game/entities/NPC.h"
#include "game/entities/Player.h"

class Shader;
class Mesh;

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
    std::vector<NPC> npcs;
    std::vector<Enemy> enemies;
    Player player;
    float worldClock = 0.0f;
    bool nightTime = false;
    bool playerKilled = false;

    Mesh* npcMesh = nullptr;
    Mesh* enemyMesh = nullptr;
    Shader* characterShader = nullptr;

    void InitializeCharacters();
    void UpdateTimeOfDay(float dt);
    void RenderCharacterMesh(const Camera& camera, float aspectRatio, const Mesh& mesh, const glm::vec3& position, const glm::vec3& scale, const glm::vec3& tint) const;
};