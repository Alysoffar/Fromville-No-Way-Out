#include "game/Game.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <iostream>
#include <sstream>

#include "engine/core/Engine.h"
#include "game/world/World.h"
#include "game/entities/Character.h"
#include "engine/renderer/Camera.h"
#include "game/quest/Quest.h"
#include "game/quest/QuestSystem.h"

Game::Game() = default;

bool Game::Initialize(Engine& engine) {
    camera = std::make_unique<Camera>();
    camera->Reset(spawnPosition);

    world = std::make_unique<World>();
    world->Initialize();

    hudRenderer = std::make_unique<TextRenderer>();
    if (!hudRenderer->Initialize("", 24)) {
        std::cerr << "[HUD] Falling back to no on-screen text overlay.\n";
        hudRenderer.reset();
    }

    engine.GetInput().SetCursorLocked(true);
    return true;
}

void Game::UpdateHudTitle(Engine& engine) const {
    if (!world) {
        return;
    }

    const Character* activeChar = world->GetActiveCharacter();
    const QuestSystem* questSystem = world->GetQuestSystem();

    std::ostringstream title;
    title << "Fromville | ";

    if (activeChar) {
        title << activeChar->GetName();
        if (activeChar->IsCrouching()) {
            title << " [Crouch]";
        } else if (activeChar->IsSprinting()) {
            title << " [Sprint]";
        }
    } else {
        title << "No active character";
    }

    if (questSystem) {
        title << " | Day " << questSystem->GetDayNumber();

        switch (questSystem->GetCurrentPhase()) {
            case StoryPhase::Exploration:    title << " | Phase: Exploration"; break;
            case StoryPhase::Revelation:     title << " | Phase: Revelation"; break;
            case StoryPhase::Confrontation:  title << " | Phase: Confrontation"; break;
            case StoryPhase::Climax:         title << " | Phase: Climax"; break;
            case StoryPhase::Epilogue:       title << " | Phase: Epilogue"; break;
        }
    }

    const std::string interactionPrompt = world->GetInteractionPrompt();
    if (!interactionPrompt.empty()) {
        title << " | " << interactionPrompt << " (E)";
    }

    title << " | 1-5 switch | WASD move | Q ability | Space jump | C crouch | Shift sprint";
    engine.GetWindow().SetTitle(title.str());
}

void Game::RenderHud(const Engine& engine) const {
    if (!hudRenderer || !world) {
        return;
    }

    const Character* activeChar = world->GetActiveCharacter();
    const QuestSystem* questSystem = world->GetQuestSystem();
    const std::string interactionPrompt = world->GetInteractionPrompt();
    const int width = engine.GetWindow().GetWidth();
    const int height = engine.GetWindow().GetHeight();

    std::ostringstream line1;
    line1 << "ACTIVE: ";
    if (activeChar) {
        line1 << activeChar->GetName() << "  HP " << static_cast<int>(activeChar->GetHealth());
        if (activeChar->IsCrouching()) {
            line1 << "  CROUCH";
        } else if (activeChar->IsSprinting()) {
            line1 << "  SPRINT";
        }
    } else {
        line1 << "NONE";
    }

    std::ostringstream line2;
    if (questSystem) {
        line2 << "DAY " << static_cast<int>(questSystem->GetDayNumber()) + 1 << "  ";
        line2 << "PHASE ";
        switch (questSystem->GetCurrentPhase()) {
            case StoryPhase::Exploration:   line2 << "EXPLORATION"; break;
            case StoryPhase::Revelation:    line2 << "REVELATION"; break;
            case StoryPhase::Confrontation: line2 << "CONFRONTATION"; break;
            case StoryPhase::Climax:        line2 << "CLIMAX"; break;
            case StoryPhase::Epilogue:      line2 << "EPILOGUE"; break;
        }
    }

    std::ostringstream line3;
    if (!interactionPrompt.empty()) {
        line3 << interactionPrompt << "  [E]";
    } else {
        line3 << "NO INTERACTION IN RANGE";
    }

    hudRenderer->RenderText("FROMVILLE TEST ARENA", 24.0f, static_cast<float>(height) - 30.0f, 0.7f, glm::vec3(0.96f, 0.93f, 0.78f), width, height);
    hudRenderer->RenderText(line1.str(), 24.0f, static_cast<float>(height) - 62.0f, 0.55f, glm::vec3(0.92f, 0.96f, 1.0f), width, height);
    hudRenderer->RenderText(line2.str(), 24.0f, static_cast<float>(height) - 92.0f, 0.50f, glm::vec3(0.74f, 0.88f, 0.78f), width, height);
    hudRenderer->RenderText(line3.str(), 24.0f, static_cast<float>(height) - 122.0f, 0.52f, glm::vec3(1.0f, 0.92f, 0.58f), width, height);

    if (activeChar && questSystem) {
        const Quest* quest = questSystem->GetCharacterQuest(activeChar->GetType());
        if (quest) {
            std::ostringstream questLine;
            questLine << "QUEST: " << quest->GetTitle() << "  " << static_cast<int>(quest->GetProgress()) << "%";
            hudRenderer->RenderText(questLine.str(), 24.0f, static_cast<float>(height) - 152.0f, 0.46f, glm::vec3(0.72f, 0.90f, 1.0f), width, height);

            const auto& objectives = quest->GetObjectives();
            for (std::size_t objectiveIndex = 0; objectiveIndex < objectives.size(); ++objectiveIndex) {
                if (!objectives[objectiveIndex].completed) {
                    std::ostringstream objectiveLine;
                    objectiveLine << "NEXT: " << objectives[objectiveIndex].description;
                    hudRenderer->RenderText(objectiveLine.str(), 24.0f, static_cast<float>(height) - 178.0f, 0.42f, glm::vec3(0.94f, 0.94f, 0.82f), width, height);
                    break;
                }
            }
        }
    }
    
    // Display NPC dialogue (center-screen, large yellow-green text)
    if (world->GetNpcDialogueDisplayTime() > 0.0f) {
        const std::string& npcMsg = world->GetLastNpcDialogue();
        hudRenderer->RenderText(npcMsg, 36.0f, static_cast<float>(height) / 2.0f - 80.0f, 0.9f, glm::vec3(0.85f, 1.0f, 0.65f), width, height);
    }
    
    // Display monster scream message (center-screen, large, intense red)
    if (world->GetMonsterScreamDisplayTime() > 0.0f) {
        const std::string& screamMsg = world->GetLastMonsterScream();
        hudRenderer->RenderText(screamMsg, 48.0f, static_cast<float>(height) / 2.0f, 0.9f, glm::vec3(1.0f, 0.2f, 0.2f), width, height);
    }
    
    // Display damage taken message (center-right, red damage indicator)
    if (world->GetLastDamageDisplayTime() > 0.0f) {
        std::ostringstream damageMsg;
        damageMsg << "DAMAGED -" << static_cast<int>(world->GetLastDamageAmount()) << " HP";
        hudRenderer->RenderText(damageMsg.str(), 36.0f, static_cast<float>(height) / 2.0f + 100.0f, 0.85f, glm::vec3(1.0f, 0.3f, 0.3f), width, height);
    }
    
    hudRenderer->RenderText("1-5 SWITCH  WASD MOVE  SPACE JUMP  C CROUCH  SHIFT SPRINT  Q ABILITY  E INTERACT", 24.0f, 28.0f, 0.40f, glm::vec3(0.82f, 0.82f, 0.82f), width, height);
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

    if (engine.GetInput().IsKeyPressed(GLFW_KEY_F5)) {
        world->SaveToFile("savegame.txt");
    }

    if (engine.GetInput().IsKeyPressed(GLFW_KEY_F9)) {
        world->LoadFromFile("savegame.txt");
    }

    if (engine.GetInput().IsKeyPressed(GLFW_KEY_LEFT_ALT)) {
        sprintToggled = !sprintToggled;
    }

    if (engine.GetInput().IsKeyPressed(GLFW_KEY_1)) {
        world->SwitchCharacter(0);
        if (Character* activeChar = world->GetActiveCharacter()) {
            camera->SnapToTarget(activeChar->transform.position);
            lastInteractionPrompt.clear();
        }
    } else if (engine.GetInput().IsKeyPressed(GLFW_KEY_2)) {
        world->SwitchCharacter(1);
        if (Character* activeChar = world->GetActiveCharacter()) {
            camera->SnapToTarget(activeChar->transform.position);
            lastInteractionPrompt.clear();
        }
    } else if (engine.GetInput().IsKeyPressed(GLFW_KEY_3)) {
        world->SwitchCharacter(2);
        if (Character* activeChar = world->GetActiveCharacter()) {
            camera->SnapToTarget(activeChar->transform.position);
            lastInteractionPrompt.clear();
        }
    } else if (engine.GetInput().IsKeyPressed(GLFW_KEY_4)) {
        world->SwitchCharacter(3);
        if (Character* activeChar = world->GetActiveCharacter()) {
            camera->SnapToTarget(activeChar->transform.position);
            lastInteractionPrompt.clear();
        }
    } else if (engine.GetInput().IsKeyPressed(GLFW_KEY_5)) {
        world->SwitchCharacter(4);
        if (Character* activeChar = world->GetActiveCharacter()) {
            camera->SnapToTarget(activeChar->transform.position);
            lastInteractionPrompt.clear();
        }
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

    Character* activeChar = world->GetActiveCharacter();
    if (activeChar) {
        activeChar->Move(moveDir.x, moveDir.z, dt);
        if (glm::length(moveDir) > 0.001f) {
            activeChar->transform.rotation.y = glm::degrees(std::atan2(moveDir.x, moveDir.z));
        }

        if (engine.GetInput().IsKeyPressed(GLFW_KEY_SPACE)) {
            activeChar->Jump();
        }
        if (engine.GetInput().IsKeyReleased(GLFW_KEY_SPACE)) {
            activeChar->ReleaseJump();
        }
        activeChar->Crouch(engine.GetInput().IsKeyDown(GLFW_KEY_C));
        const bool sprintHeld = engine.GetInput().IsKeyDown(GLFW_KEY_LEFT_SHIFT);
        activeChar->Sprint(sprintHeld || sprintToggled);

        if (engine.GetInput().IsKeyPressed(GLFW_KEY_E)) {
            world->TryActiveCharacterInteraction();
        }

        if (engine.GetInput().IsKeyPressed(GLFW_KEY_Q)) {
            activeChar->ActivateAbility();
        }

        const std::string interactionPrompt = world->GetInteractionPrompt();
        if (interactionPrompt != lastInteractionPrompt) {
            lastInteractionPrompt = interactionPrompt;
            if (!interactionPrompt.empty()) {
                std::cout << "[Interact] " << interactionPrompt << " (E)\n";
            }
        }

        camera->Update(engine.GetInput(), dt, activeChar->transform.position);
    }
    world->Update(*camera, dt);
}

void Game::Render(Engine& engine) const {
    if (!camera || !world) {
        return;
    }

    world->Render(*camera, engine.GetAspectRatio());
    RenderHud(engine);
}

void Game::Shutdown() {
    hudRenderer.reset();
    world.reset();
    camera.reset();
}
