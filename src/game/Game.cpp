#include "game/Game.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <cmath>
#include <iostream>
#include <sstream>

#include "engine/core/Engine.h"
#include "engine/input/InputAction.h"
#include "engine/renderer/Camera.h"
#include "game/entities/Character.h"
#include "game/quest/Quest.h"
#include "game/quest/QuestSystem.h"
#include "game/world/World.h"

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
            case StoryPhase::Exploration:   title << " | Phase: Exploration"; break;
            case StoryPhase::Revelation:     title << " | Phase: Revelation"; break;
            case StoryPhase::Confrontation:  title << " | Phase: Confrontation"; break;
            case StoryPhase::Climax:         title << " | Phase: Climax"; break;
            case StoryPhase::Epilogue:       title << " | Phase: Epilogue"; break;
        }
    }

    const std::string interactionPrompt = world->GetInteractionPrompt();
    if (!interactionPrompt.empty()) {
        if (world->NearestInteractionIsPickup()) {
            title << " | " << interactionPrompt << " (F)";
        } else {
            title << " | " << interactionPrompt << " (E)";
        }
    }

    title << " | 1-5 switch | WASD move | E interact | Q abandon quest | Space jump | C crouch | Shift sprint";
    engine.GetWindow().SetTitle(title.str());
}

void Game::RenderHud(const Engine& engine) const {
    if (!hudRenderer || !world) {
        return;
    }

    if (world->IsPuzzleActive()) {
        return;
    }

    const Character* activeChar = world->GetActiveCharacter();
    const QuestSystem* questSystem = world->GetQuestSystem();
    const std::string interactionPrompt = world->GetInteractionPrompt();
    const std::string questHelper = world->GetQuestHelperText();
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
        line2 << "DAY " << static_cast<int>(questSystem->GetDayNumber()) + 1 << "  PHASE ";
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

    std::ostringstream line4;
    if (!questHelper.empty()) {
        line4 << questHelper;
    }

    hudRenderer->RenderText("FROMVILLE TEST ARENA", 24.0f, static_cast<float>(height) - 30.0f, 0.7f, glm::vec3(0.96f, 0.93f, 0.78f), width, height);
    hudRenderer->RenderText(line1.str(), 24.0f, static_cast<float>(height) - 62.0f, 0.55f, glm::vec3(0.92f, 0.96f, 1.0f), width, height);
    hudRenderer->RenderText(line2.str(), 24.0f, static_cast<float>(height) - 92.0f, 0.50f, glm::vec3(0.74f, 0.88f, 0.78f), width, height);

    std::ostringstream promptFormatted;
    if (!interactionPrompt.empty()) {
        promptFormatted << "[E] " << interactionPrompt;
    } else {
        promptFormatted << "No interaction nearby";
    }
    hudRenderer->RenderText(promptFormatted.str(), 24.0f, static_cast<float>(height) - 140.0f, 0.58f, glm::vec3(1.0f, 1.0f, 0.3f), width, height);

    if (!questHelper.empty()) {
        hudRenderer->RenderText("HELP:", 26.0f, static_cast<float>(height) - 190.0f, 0.48f, glm::vec3(1.0f, 0.6f, 0.2f), width, height);
        hudRenderer->RenderText(line4.str(), 26.0f, static_cast<float>(height) - 220.0f, 0.50f, glm::vec3(1.0f, 0.5f, 0.2f), width, height);
    }

    const std::string waypointText = world->GetQuestWaypointText();
    if (!waypointText.empty()) {
        hudRenderer->RenderText("NEXT POINT: " + waypointText, 26.0f, static_cast<float>(height) - 250.0f, 0.56f, glm::vec3(0.78f, 0.32f, 1.0f), width, height);
        hudRenderer->RenderText(waypointText, 26.0f, static_cast<float>(height) - 278.0f, 0.48f, glm::vec3(0.3f, 1.0f, 0.8f), width, height);
    }

    if (activeChar && questSystem) {
        const Quest* quest = questSystem->GetCharacterQuest(activeChar->GetType());
        if (quest) {
            std::ostringstream questLine;
            const bool isActiveQuest = world->HasActiveQuest() && world->GetActiveQuestCharacter() == activeChar->GetType();
            if (isActiveQuest) {
                questLine << ">>> ACTIVE QUEST: " << quest->GetTitle() << "  " << static_cast<int>(quest->GetProgress()) << "% <<<";
                hudRenderer->RenderText(questLine.str(), 28.0f, static_cast<float>(height) - 152.0f, 0.48f, glm::vec3(1.0f, 0.8f, 0.2f), width, height);
            } else {
                questLine << "QUEST: " << quest->GetTitle() << "  " << static_cast<int>(quest->GetProgress()) << "%";
                hudRenderer->RenderText(questLine.str(), 24.0f, static_cast<float>(height) - 152.0f, 0.46f, glm::vec3(0.72f, 0.90f, 1.0f), width, height);
            }

            const auto& objectives = quest->GetObjectives();
            for (std::size_t objectiveIndex = 0; objectiveIndex < objectives.size(); ++objectiveIndex) {
                if (!objectives[objectiveIndex].completed) {
                    std::ostringstream objectiveLine;
                    objectiveLine << "NEXT: " << objectives[objectiveIndex].description;
                    hudRenderer->RenderText(objectiveLine.str(), 24.0f, static_cast<float>(height) - 178.0f, 0.42f, glm::vec3(0.94f, 0.94f, 0.82f), width, height);
                    break;
                }
            }

            if (isActiveQuest) {
                const int nextObjIndex = quest->GetNextIncompleteObjectiveIndex();
                if (nextObjIndex >= 0 && nextObjIndex < static_cast<int>(objectives.size())) {
                    const QuestObjective& currentObj = objectives[nextObjIndex];
                    const float currentY = static_cast<float>(height) - 230.0f;

                    std::ostringstream typeLine;
                    typeLine << "[";
                    switch (currentObj.type) {
                        case ObjectiveType::Dialogue:      typeLine << "TALK"; break;
                        case ObjectiveType::Collect:       typeLine << "COLLECT"; break;
                        case ObjectiveType::Puzzle:        typeLine << "PUZZLE"; break;
                        case ObjectiveType::Observe:       typeLine << "OBSERVE"; break;
                        case ObjectiveType::Combat:        typeLine << "COMBAT"; break;
                        case ObjectiveType::Timed:         typeLine << "TIMED"; break;
                        case ObjectiveType::Skill:         typeLine << "SKILL"; break;
                        case ObjectiveType::Environmental: typeLine << "SEARCH"; break;
                    }
                    typeLine << "]";
                    hudRenderer->RenderText(typeLine.str(), 20.0f, currentY, 0.45f, glm::vec3(0.4f, 1.0f, 0.4f), width, height);

                    const std::string progress = currentObj.GetProgressString();
                    if (!progress.empty()) {
                        std::ostringstream progressLine;
                        progressLine << "Progress: " << progress;
                        hudRenderer->RenderText(progressLine.str(), 20.0f, currentY - 20.0f, 0.40f, glm::vec3(1.0f, 0.8f, 0.4f), width, height);
                    }

                    const std::string hint = quest->GetCurrentObjectiveHint(nextObjIndex);
                    if (!hint.empty()) {
                        std::ostringstream hintLine;
                        hintLine << "TIP: " << hint;
                        hudRenderer->RenderText(hintLine.str(), 20.0f, currentY - 40.0f, 0.38f, glm::vec3(1.0f, 1.0f, 0.6f), width, height);
                    }

                    const auto& clues = quest->GetObjectiveClues(nextObjIndex);
                    float clueY = currentY - 65.0f;
                    for (std::size_t i = 0; i < clues.size() && i < 2; ++i) {
                        std::ostringstream clueLine;
                        clueLine << "CLUE: " << clues[i];
                        hudRenderer->RenderText(clueLine.str(), 20.0f, clueY - (static_cast<float>(i) * 18.0f), 0.35f, glm::vec3(0.7f, 0.9f, 1.0f), width, height);
                    }

                    const auto& dialogues = quest->GetObjectiveDialogues(nextObjIndex);
                    float dialogY = currentY - 110.0f;
                    for (std::size_t i = 0; i < dialogues.size() && i < 1; ++i) {
                        if (dialogues[i].revealed) {
                            std::ostringstream dialogLine;
                            dialogLine << "DIALOGUE: " << dialogues[i].speaker << " - " << dialogues[i].text.substr(0, 50) << "...";
                            hudRenderer->RenderText(dialogLine.str(), 20.0f, dialogY, 0.34f, glm::vec3(0.8f, 0.9f, 1.0f), width, height);
                        }
                    }
                }
            }
        }
    }

    if (world->GetNpcDialogueDisplayTime() > 0.0f) {
        const std::string& npcMsg = world->GetLastNpcDialogue();
        hudRenderer->RenderText(npcMsg, 36.0f, static_cast<float>(height) / 2.0f - 80.0f, 0.9f, glm::vec3(0.85f, 1.0f, 0.65f), width, height);
    }

    if (world->GetMonsterScreamDisplayTime() > 0.0f) {
        const std::string& screamMsg = world->GetLastMonsterScream();
        hudRenderer->RenderText(screamMsg, 48.0f, static_cast<float>(height) / 2.0f, 0.9f, glm::vec3(1.0f, 0.2f, 0.2f), width, height);
    }

    if (world->GetLastDamageDisplayTime() > 0.0f) {
        std::ostringstream damageMsg;
        damageMsg << "DAMAGED -" << static_cast<int>(world->GetLastDamageAmount()) << " HP";
        hudRenderer->RenderText(damageMsg.str(), 36.0f, static_cast<float>(height) / 2.0f + 100.0f, 0.85f, glm::vec3(1.0f, 0.3f, 0.3f), width, height);
    }

    if (world->GetLastInteractionFeedbackTime() > 0.0f) {
        const std::string& feedbackMsg = world->GetLastInteractionFeedback();
        hudRenderer->RenderText(feedbackMsg, 32.0f, static_cast<float>(height) / 2.0f + 50.0f, 0.88f, glm::vec3(0.2f, 1.0f, 0.3f), width, height);
    }

    hudRenderer->RenderText("1-4 SWITCH  WASD MOVE  SPACE JUMP  C CROUCH  SHIFT SPRINT  Q ABANDON QUEST  E INTERACT", 24.0f, 28.0f, 0.40f, glm::vec3(0.82f, 0.82f, 0.82f), width, height);

    if (!interactionPrompt.empty() && world) {
        const bool isPickup = world->NearestInteractionIsPickup();
        const std::string key = isPickup ? "[F] " : "[E] ";
        const std::string centerPrompt = key + interactionPrompt;
        hudRenderer->RenderText(centerPrompt, static_cast<float>(width) * 0.12f, 120.0f, 1.0f, glm::vec3(1.0f, 0.9f, 0.6f), width, height);
    }
}

void Game::HandleGlobalInput(Engine& engine) {
    const InputContext& input = engine.GetGameplayInput();

    if (input.IsActionPressed(InputAction::Pause)) {
        cursorLocked = !cursorLocked;
        engine.GetInput().SetCursorLocked(cursorLocked);
    }

    if (input.IsActionPressed(InputAction::ResetView)) {
        camera->Reset(spawnPosition);
    }

    if (input.IsActionPressed(InputAction::SaveGame)) {
        world->SaveToFile("savegame.txt");
    }

    if (input.IsActionPressed(InputAction::LoadGame)) {
        world->LoadFromFile("savegame.txt");
    }
}

void Game::HandleGameplayInput(float dt, Engine& engine) {
    const InputContext& input = engine.GetGameplayInput();

    if (input.IsActionPressed(InputAction::Sprint)) {
        sprintToggled = !sprintToggled;
    }

    if (input.IsActionPressed(InputAction::SwitchCharacter1)) {
        world->SwitchCharacter(0);
    } else if (input.IsActionPressed(InputAction::SwitchCharacter2)) {
        world->SwitchCharacter(1);
    } else if (input.IsActionPressed(InputAction::SwitchCharacter3)) {
        world->SwitchCharacter(2);
    } else if (input.IsActionPressed(InputAction::SwitchCharacter4)) {
        world->SwitchCharacter(3);
    }

    Character* activeChar = world->GetActiveCharacter();
    if (!activeChar) {
        return;
    }

    if (input.IsActionPressed(InputAction::Jump)) {
        activeChar->Jump();
    }
    if (input.IsActionReleased(InputAction::Jump)) {
        activeChar->ReleaseJump();
    }
    activeChar->Crouch(input.IsActionDown(InputAction::Crouch));
    activeChar->Sprint(input.IsActionDown(InputAction::Sprint) || sprintToggled);

    if (input.IsActionPressed(InputAction::Interact)) {
        world->TryActiveCharacterInteraction();
    }
    if (input.IsActionPressed(InputAction::Pickup)) {
        world->TryActiveCharacterPickup();
    }
    if (input.IsActionPressed(InputAction::AbandonQuest)) {
        world->AbandonActiveQuest();
    }

    const glm::vec2 movement = input.GetMovementVector();
    glm::vec3 camForward = camera->GetForward();
    camForward.y = 0.0f;
    if (glm::length(camForward) > 0.001f) {
        camForward = glm::normalize(camForward);
    }

    glm::vec3 camRight = camera->GetRight();
    camRight.y = 0.0f;
    if (glm::length(camRight) > 0.001f) {
        camRight = glm::normalize(camRight);
    }

    const glm::vec3 moveDir = camForward * movement.y + camRight * movement.x;
    activeChar->Move(moveDir.x, moveDir.z, dt);
    if (glm::length(moveDir) > 0.001f) {
        activeChar->transform.rotation.y = glm::degrees(std::atan2(moveDir.x, moveDir.z));
    }

    camera->Update(engine.GetInput(), dt, activeChar->transform.position);
}

void Game::HandleCharacterInput(float dt, Engine& engine) {
    if (world->IsPuzzleActive()) {
        world->UpdatePuzzle(dt, engine.GetUiInput());
        if (world->ConsumeSpawnRestartRequest()) {
            camera->Reset(spawnPosition);
            lastInteractionPrompt.clear();
        }
        return;
    }

    HandleGameplayInput(dt, engine);
}

void Game::Update(float dt, Engine& engine) {
    if (!camera || !world) {
        return;
    }

    HandleGlobalInput(engine);

    const InputContext& input = engine.GetGameplayInput();
    if (input.IsActionPressed(InputAction::SwitchCharacter1) ||
        input.IsActionPressed(InputAction::SwitchCharacter2) ||
        input.IsActionPressed(InputAction::SwitchCharacter3) ||
        input.IsActionPressed(InputAction::SwitchCharacter4) ||
        input.IsActionPressed(InputAction::SwitchCharacter5)) {
        if (Character* activeChar = world->GetActiveCharacter()) {
            camera->SnapToTarget(activeChar->transform.position);
            lastInteractionPrompt.clear();
        }
    }

    HandleCharacterInput(dt, engine);

    const std::string interactionPrompt = world->GetInteractionPrompt();
    if (interactionPrompt != lastInteractionPrompt) {
        lastInteractionPrompt = interactionPrompt;
        if (!interactionPrompt.empty()) {
            std::cout << "[Interact] " << interactionPrompt << " (E)\n";
        }
    }

    world->Update(*camera, dt);
}

void Game::Render(Engine& engine) const {
    if (!camera || !world) {
        return;
    }

    world->Render(*camera, engine.GetAspectRatio());
    if (hudRenderer) {
        if (!world->IsPuzzleActive()) {
            world->RenderNarrativeOverlays(*hudRenderer, engine.GetWindow().GetWidth(), engine.GetWindow().GetHeight());
        }
        world->RenderPuzzleOverlay(*hudRenderer, engine.GetWindow().GetWidth(), engine.GetWindow().GetHeight());
    }
    RenderHud(engine);
}

void Game::Shutdown() {
    hudRenderer.reset();
    world.reset();
    camera.reset();
}