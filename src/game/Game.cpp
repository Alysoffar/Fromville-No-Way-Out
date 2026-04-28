#include "game/Game.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "engine/core/Engine.h"
#include "game/world/World.h"
#include "engine/renderer/Camera.h"

Game::Game() = default;

bool Game::Initialize(Engine& engine) {
    camera = std::make_unique<Camera>();
    camera->Reset(spawnPosition);

    world = std::make_unique<World>();
    world->Initialize();

    engine.GetInput().SetCursorLocked(true);
    return true;
}

void Game::Update(float dt, Engine& engine) {
    if (!camera || !world) {
        return;
    }

    if (engine.GetInput().IsKeyPressed(GLFW_KEY_ESCAPE)) {
        cursorLocked = !cursorLocked;
        engine.GetInput().SetCursorLocked(cursorLocked);
    }

    if (engine.GetInput().IsKeyPressed(GLFW_KEY_R)) {
        camera->Reset(spawnPosition);
    }

    camera->Update(engine.GetInput(), dt);
    world->Update(*camera, dt);
}

void Game::Render(Engine& engine) const {
    if (!camera || !world) {
        return;
    }

    world->Render(*camera, engine.GetAspectRatio());
}

void Game::Shutdown() {
    world.reset();
    camera.reset();
}