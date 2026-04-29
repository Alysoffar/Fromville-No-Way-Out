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

    // Get camera's forward and right vectors, flattened to the XZ plane
    glm::vec3 camForward = camera->GetForward();
    camForward.y = 0.0f;
    if (glm::length(camForward) > 0.001f) camForward = glm::normalize(camForward);

    glm::vec3 camRight = camera->GetRight();
    camRight.y = 0.0f;
    if (glm::length(camRight) > 0.001f) camRight = glm::normalize(camRight);

    // Build a movement direction relative to the camera
    glm::vec3 moveDir(0.0f);
    if (engine.GetInput().IsKeyDown(GLFW_KEY_W)) moveDir += camForward;
    if (engine.GetInput().IsKeyDown(GLFW_KEY_S)) moveDir -= camForward;
    if (engine.GetInput().IsKeyDown(GLFW_KEY_D)) moveDir += camRight;
    if (engine.GetInput().IsKeyDown(GLFW_KEY_A)) moveDir -= camRight;

    if (glm::length(moveDir) > 0.001f) {
        moveDir = glm::normalize(moveDir);
    }

    world->GetPlayer().Move(moveDir.x, moveDir.z, dt);

    if (engine.GetInput().IsKeyPressed(GLFW_KEY_SPACE)) {
        world->GetPlayer().Jump();
    }
    world->GetPlayer().Crouch(engine.GetInput().IsKeyDown(GLFW_KEY_C));
    world->GetPlayer().Sprint(engine.GetInput().IsKeyDown(GLFW_KEY_LEFT_SHIFT));

    camera->Update(engine.GetInput(), dt, world->GetPlayer().transform.position);
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