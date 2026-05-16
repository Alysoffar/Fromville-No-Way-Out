#include "game/Game.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <sstream>

#include "engine/core/Engine.h"
#include "game/world/World.h"
#include "game/world/DayNightCycle.h"
#include "game/dialogue/DialogueManager.h"
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
    textRenderer.Initialize("C:/Windows/Fonts/arial.ttf", 24);
    
    // Configure Input
    engine.GetGameplayInput().Attach(&engine.GetInput());
    engine.GetGameplayInput().ConfigureGameplayDefaults();
    engine.GetUiInput().Attach(&engine.GetInput());
    engine.GetUiInput().ConfigureUiDefaults();

    DialogueManager::Instance().SetAudioManager(world->GetAudioManager());
    DialogueManager::Instance().Initialize();
    DialogueManager::Instance().LoadConversationsFromFolder("assets/dialogue");

    // Load initial sounds
    if (auto* audio = world->GetAudioManager()) {
        audio->LoadSound("ambient_loop", "assets/audio/music/ambient_tension_low.wav");
        audio->PlaySound("ambient_loop", 0.3f);
    }

    engine.GetInput().SetCursorLocked(true);
    return true;
}

void Game::Update(float dt, Engine& engine) {
    if (!camera || !world) return;

    const auto& input = engine.GetGameplayInput();

    if (input.IsActionPressed(InputAction::Pause)) {
        cursorLocked = !cursorLocked;
        engine.GetInput().SetCursorLocked(cursorLocked);
    }

    if (input.IsActionPressed(InputAction::ResetView)) {
        camera->Reset(spawnPosition);
    }

    // Character Switching
    if (input.IsActionPressed(InputAction::SwitchCharacter1)) world->SwitchCharacter(0);
    if (input.IsActionPressed(InputAction::SwitchCharacter2)) world->SwitchCharacter(1);
    if (input.IsActionPressed(InputAction::SwitchCharacter3)) world->SwitchCharacter(2);
    if (input.IsActionPressed(InputAction::SwitchCharacter4)) world->SwitchCharacter(3);
    if (input.IsActionPressed(InputAction::SwitchCharacter5)) world->SwitchCharacter(4);

    // Ability Trigger
    if (input.IsActionPressed(InputAction::Ability)) {
        world->GetPlayer().ActivateAbility();
    }

    // Update camera
    camera->Update(engine.GetInput(), dt, world->GetPlayer().transform.position);

    const bool canMovePlayer = (camera->GetState() == Camera::CameraState::FIRST_PERSON);

    if (canMovePlayer) {
        glm::vec2 moveVec = input.GetMovementVector();
        glm::vec3 camForward = camera->GetForward();
        camForward.y = 0.0f;
        if (glm::length(camForward) > 0.001f) camForward = glm::normalize(camForward);

        glm::vec3 camRight = camera->GetRight();
        camRight.y = 0.0f;
        if (glm::length(camRight) > 0.001f) camRight = glm::normalize(camRight);

        glm::vec3 moveDir = camForward * moveVec.y + camRight * moveVec.x;
        world->GetPlayer().Move(moveDir.x, moveDir.z, dt);
    }

    if (input.IsActionPressed(InputAction::Jump)) {
        world->GetPlayer().Jump();
    }
    world->GetPlayer().Crouch(input.IsActionDown(InputAction::Crouch));
    world->GetPlayer().Sprint(input.IsActionDown(InputAction::Sprint));

    if (input.IsActionPressed(InputAction::Interact)) {
        world->TryInteract();
    }

    world->Update(*camera, dt, engine.GetInput());
    dayNightCycle.update(dt);
    UpdateHudTitle(engine);

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

    // 5. Render characters
    if (characterMesh && animatedShader) {
        animatedShader->Bind();
        animatedShader->SetMat4("view", view);
        animatedShader->SetMat4("projection", projection);
        animatedShader->SetVec3("lightDir", dayNightCycle.getActiveLightDir());
        animatedShader->SetVec3("lightColor", dayNightCycle.getLightColor());
        animatedShader->SetVec3("viewPos", cameraPos);
        animatedShader->SetFloat("fogDensity", fogDensity);
        animatedShader->SetVec3("fogColor", dayNightCycle.getFogColor());

        const uint32_t characterLayer = static_cast<uint32_t>(Camera::RenderLayer::LAYER_PLAYER);

        // Render all characters
        for (auto& character : world->GetCharacters()) {
            // If it's the active character and we are in first-person, we might want to hide it
            if (character.get() == &world->GetPlayer()) {
                if ((characterLayer & camera->cullingMask) == 0u) continue;
            }

            glm::vec3 pos = character->transform.position;
            float rot = character->transform.rotation.y;

            glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
            model = glm::rotate(model, glm::radians(rot), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(0.01f));
            animatedShader->SetMat4("model", model);

            if (animator) {
                animatedShader->SetMat4Array("finalBonesMatrices", animator->GetFinalBoneMatrices());
            }

            characterMesh->draw(animatedShader->GetID());
        }
    }

    // 6. UI / Gameplay Overlays
    int width, height;
    glfwGetFramebufferSize(engine.GetWindow().GetHandle(), &width, &height);

    if (world->IsPuzzleActive()) {
        world->RenderPuzzleOverlay(textRenderer, width, height);
    }

    if (DialogueManager::Instance().IsActive()) {
        DialogueManager::Instance().Render(&textRenderer, width, height);
    }
    
    RenderHud(engine);
}

void Game::UpdateHudTitle(Engine& engine) const {
    if (!world) return;
    const Character* activeChar = world->GetActiveCharacter();
    std::ostringstream title;
    title << "Fromville | ";
    if (activeChar) title << activeChar->GetName();
    engine.GetWindow().SetTitle(title.str());
}

void Game::RenderHud(const Engine& engine) const {
    if (!world) return;
    int width, height;
    glfwGetFramebufferSize(engine.GetWindow().GetHandle(), &width, &height);

    const Character* activeChar = world->GetActiveCharacter();
    if (activeChar) {
        std::string nameInfo = "ACTIVE: " + activeChar->GetName() + "  HP: " + std::to_string(static_cast<int>(activeChar->GetHealth()));
        textRenderer.RenderText(nameInfo, 24.0f, static_cast<float>(height) - 40.0f, 0.5f, glm::vec3(1.0f), width, height);
    }

    std::string prompt = world->GetInteractionPrompt();
    if (!prompt.empty()) {
        textRenderer.RenderText("[E] " + prompt, width / 2.0f - 100.0f, height / 2.0f - 50.0f, 0.6f, glm::vec3(1.0f, 1.0f, 0.3f), width, height);
    }

    std::string questInfo = world->GetQuestHelperText();
    if (!questInfo.empty()) {
        textRenderer.RenderText("QUEST: " + questInfo, 24.0f, 40.0f, 0.45f, glm::vec3(0.3f, 1.0f, 0.8f), width, height);
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
