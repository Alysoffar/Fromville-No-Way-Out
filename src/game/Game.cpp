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
        // Show correct key hint: use F for pickups and E for general interactions
        if (world && world->NearestInteractionIsPickup()) {
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

    // When a puzzle modal is active, hide the normal HUD to avoid text overlap
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

    std::ostringstream line4;
    if (!questHelper.empty()) {
        line4 << questHelper;
    }

    hudRenderer->RenderText("FROMVILLE TEST ARENA", 24.0f, static_cast<float>(height) - 30.0f, 0.7f, glm::vec3(0.96f, 0.93f, 0.78f), width, height);
    hudRenderer->RenderText(line1.str(), 24.0f, static_cast<float>(height) - 62.0f, 0.55f, glm::vec3(0.92f, 0.96f, 1.0f), width, height);
    hudRenderer->RenderText(line2.str(), 24.0f, static_cast<float>(height) - 92.0f, 0.50f, glm::vec3(0.74f, 0.88f, 0.78f), width, height);
    
    // Interaction prompt with shapes for distinction
    std::ostringstream promptFormatted;
    if (!interactionPrompt.empty()) {
        promptFormatted << "▶ [E] " << interactionPrompt;
    } else {
        promptFormatted << "▶ No interaction nearby";
    }
    hudRenderer->RenderText(promptFormatted.str(), 24.0f, static_cast<float>(height) - 140.0f, 0.58f, glm::vec3(1.0f, 1.0f, 0.3f), width, height);
    
    // Quest helper text with clear label
    if (!questHelper.empty()) {
        hudRenderer->RenderText("HELP:", 26.0f, static_cast<float>(height) - 190.0f, 0.48f, glm::vec3(1.0f, 0.6f, 0.2f), width, height);
        hudRenderer->RenderText(line4.str(), 26.0f, static_cast<float>(height) - 220.0f, 0.50f, glm::vec3(1.0f, 0.5f, 0.2f), width, height);
    }
    
    // Quest waypoint compass marker
    const std::string waypointText = world->GetQuestWaypointText();
    if (!waypointText.empty()) {
        const std::string directionalArrow = "➤ NEXT POINT: " + waypointText;
        hudRenderer->RenderText(directionalArrow, 26.0f, static_cast<float>(height) - 250.0f, 0.56f, glm::vec3(0.78f, 0.32f, 1.0f), width, height);
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
    
    // Display quest acceptance/abandonment feedback
    if (world->GetLastInteractionFeedbackTime() > 0.0f) {
        const std::string& feedbackMsg = world->GetLastInteractionFeedback();
        hudRenderer->RenderText(feedbackMsg, 32.0f, static_cast<float>(height) / 2.0f + 50.0f, 0.88f, glm::vec3(0.2f, 1.0f, 0.3f), width, height);
    }
    
    hudRenderer->RenderText("1-5 SWITCH  WASD MOVE  SPACE JUMP  C CROUCH  SHIFT SPRINT  Q ABANDON QUEST  E INTERACT", 24.0f, 28.0f, 0.40f, glm::vec3(0.82f, 0.82f, 0.82f), width, height);

    // When near an interactable, render a large, readable prompt in the center-bottom
    if (!interactionPrompt.empty() && world) {
        const bool isPickup = world->NearestInteractionIsPickup();
        const std::string key = isPickup ? "[F] " : "[E] ";
        const std::string centerPrompt = key + interactionPrompt;
        hudRenderer->RenderText(centerPrompt, static_cast<float>(width) * 0.12f, 120.0f, 1.0f, glm::vec3(1.0f, 0.9f, 0.6f), width, height);
    }
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

    if (world->IsPuzzleActive()) {
        world->UpdatePuzzle(dt, engine.GetInput());
        return;
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

    // If a puzzle modal is active, update puzzle input and freeze player movement/camera
    if (world && world->IsPuzzleActive()) {
        world->UpdatePuzzle(dt, engine.GetInput());
        return;
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
        // Use F for item pickups (collect glyphs/triangles)
        if (engine.GetInput().IsKeyPressed(GLFW_KEY_F)) {
            world->TryActiveCharacterPickup();
        }

        if (engine.GetInput().IsKeyPressed(GLFW_KEY_Q)) {
            world->AbandonActiveQuest();
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
    if (hudRenderer) {
        world->RenderPuzzleOverlay(*hudRenderer, engine.GetWindow().GetWidth(), engine.GetWindow().GetHeight());
    }
    RenderHud(engine);
}

void Game::Shutdown() {
    hudRenderer.reset();
    world.reset();
    camera.reset();
}
