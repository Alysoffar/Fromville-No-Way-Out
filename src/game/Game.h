#pragma once

#include <memory>

#include <glm/glm.hpp>

class Camera;
class Engine;

#include "engine/renderer/Camera.h"
#include "game/world/World.h"

class Game {
public:
    Game();

    bool Initialize(Engine& engine);
    void Update(float dt, Engine& engine);
    void Render(Engine& engine) const;
    void Shutdown();

private:
    std::unique_ptr<Camera> camera;
    std::unique_ptr<World> world;

    glm::vec3 spawnPosition = glm::vec3(0.0f, 1.8f, 3.5f);
    bool cursorLocked = true;
    bool sprintToggled = false;
};