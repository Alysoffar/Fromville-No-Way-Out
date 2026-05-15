#include "game/Game.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "engine/core/Engine.h"
#include "game/world/World.h"
#include "engine/renderer/Camera.h"

Game::Game() = default;

bool Game::Initialize(Engine& engine) {
    camera = std::make_unique<Camera>();
    camera->Reset(spawnPosition);

    world = std::make_unique<World>();
    world->Initialize();

    // Initialize new terrain rendering subsystems
    groundRenderer.init();
    grassRenderer.init();
    treeRenderer.init(world->GetCollisionWorld());
    world->GetCollisionWorld()->BuildBVH();
    skydomeRenderer.init();

    characterMesh = std::make_unique<AnimatedMesh>("assets/models/Character 1/character.fbx");
    animatedShader = std::make_unique<Shader>("animated");
    animatedShader->Load("shaders/animated.vert", "shaders/animated.frag");

    walkingAnimation = std::make_unique<Animation>("assets/models/Character 1/Standing Idle.fbx", characterMesh.get());
    animator = std::make_unique<Animator>(walkingAnimation.get());

    m_Player.init(engine.GetWindow().GetHandle());

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

    if (engine.GetInput().IsKeyPressed(GLFW_KEY_E)) {
        world->TryInteract();
    }

    camera->Update(engine.GetInput(), dt, world->GetPlayer().transform.position);
    world->Update(*camera, dt);

    // Advance day/night cycle
    dayNightCycle.update(dt);

    if (animator) {
        animator->UpdateAnimation(dt);
    }

    // Set clear color to current fog color so BeginFrame clears with the right color
    glm::vec3 fogColor = dayNightCycle.getFogColor();
    glClearColor(fogColor.r, fogColor.g, fogColor.b, 1.0f);
}

void Game::Render(Engine& engine) const {
    if (!camera || !world) {
        return;
    }

    const float aspectRatio = engine.GetAspectRatio();
    const glm::mat4 view = camera->GetViewMatrix();
    const glm::mat4 projection = camera->GetProjectionMatrix(aspectRatio);
    const glm::vec3 cameraPos = camera->GetPosition();
    const float currentTime = static_cast<float>(glfwGetTime());

    const float fogDensity = 0.018f;

    // 1. Skydome (drawn first, behind everything)
    skydomeRenderer.render(view, projection,
                           dayNightCycle.getSunDirection(),
                           dayNightCycle.getDayFactor(),
                           dayNightCycle.getDayTime());

    // 2. Textured ground quad
    groundRenderer.render(view, projection, cameraPos,
                          dayNightCycle.getActiveLightDir(),
                          dayNightCycle.getLightColor(),
                          dayNightCycle.getAmbientColor(),
                          dayNightCycle.getDiffuseStrength(),
                          dayNightCycle.getFogColor(), fogDensity);

    // 3. Instanced grass billboards
    grassRenderer.render(view, projection, cameraPos, currentTime,
                         dayNightCycle.getActiveLightDir(),
                         dayNightCycle.getLightColor(),
                         dayNightCycle.getAmbientColor(),
                         dayNightCycle.getDiffuseStrength(),
                         dayNightCycle.getFogColor(), fogDensity);

    // 3.5 Instanced trees
    treeRenderer.render(view, projection, cameraPos,
                        dayNightCycle.getActiveLightDir(),
                        dayNightCycle.getLightColor(),
                        dayNightCycle.getAmbientColor(),
                        dayNightCycle.getDiffuseStrength(),
                        dayNightCycle.getFogColor(), fogDensity);

    // 4. World objects and player (using dynamic day/night lighting)
    world->RenderObjects(*camera, aspectRatio, dayNightCycle, fogDensity);

    // 5. Render animated character
    if (characterMesh && animatedShader) {
        animatedShader->Bind();
        animatedShader->SetMat4("view", view);
        animatedShader->SetMat4("projection", projection);
        animatedShader->SetVec3("lightDir", dayNightCycle.getActiveLightDir());
        animatedShader->SetVec3("lightColor", dayNightCycle.getLightColor());
        animatedShader->SetVec3("viewPos", cameraPos);
        animatedShader->SetFloat("fogDensity", fogDensity);
        animatedShader->SetVec3("fogColor", dayNightCycle.getFogColor());

        glm::vec3 playerPos = world->GetPlayer().transform.position;
        float playerRot = world->GetPlayer().transform.rotation.y;

        glm::mat4 model = glm::translate(glm::mat4(1.0f), playerPos);
        model = glm::rotate(model, glm::radians(playerRot), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.01f)); // Mixamo FBX usually requires scaling down
        animatedShader->SetMat4("model", model);

        if (animator) {
            animatedShader->SetMat4Array("finalBonesMatrices", animator->GetFinalBoneMatrices());
        }

        characterMesh->draw(animatedShader->GetID());
    }
}

void Game::Shutdown() {
    groundRenderer.cleanup();
    grassRenderer.cleanup();
    treeRenderer.cleanup();
    skydomeRenderer.cleanup();

    world.reset();
    camera.reset();
    characterMesh.reset();
    animatedShader.reset();
}