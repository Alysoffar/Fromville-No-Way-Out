#include "game/Game.h"
#include "engine/core/StartupTimer.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <cmath>
#include <iostream>
#include <sstream>
#include <filesystem>

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "engine/core/Engine.h"
#include "engine/input/InputAction.h"
#include "engine/renderer/Camera.h"
#include "game/entities/Character.h"
#include "game/quest/Quest.h"
#include "game/quest/QuestSystem.h"
#include "game/world/World.h"

Game::Game() {}

Game::~Game() {
    Shutdown();
}

bool Game::InitializePhase0(Engine& engine) {
    camera = std::make_unique<Camera>();
    camera->Reset(spawnPosition);

    world = std::make_unique<World>();
    world->Initialize();
    return true;
}

bool Game::InitializePhase1(Engine& engine) {
    hudRenderer = std::make_unique<TextRenderer>();
    if (!hudRenderer->Initialize("", 24)) {
        std::cerr << "[HUD] Falling back to no on-screen text overlay.\n";
        hudRenderer.reset();
    }

    groundRenderer.init();
    grassRenderer.init();
    if (world) {
        treeRenderer.init(world->GetCollisionWorld());
    }
    skydomeRenderer.init();
    return true;
}

bool Game::InitializePhase2(Engine& engine) {
    animatedShader = std::make_unique<Shader>("animated");
    try {
        animatedShader->Load("shaders/animated.vert", "shaders/animated.frag");
    } catch (const std::exception& e) {
        std::cerr << "[Warning] animated shader failed to load: " << e.what() << "\n";
    }
    return true;
}

bool Game::LoadNextCharacterMesh() {
    if (m_loadedMeshCount == 0) {
        try {
            characterMesh = std::make_unique<AnimatedMesh>("assets/models/Character 1/character.fbx");
        } catch (const std::exception& e) {
            std::cerr << "[Warning] Character mesh failed to load: " << e.what() << "\n";
        }
        m_loadedMeshCount = 1;
        return true;
    } else if (m_loadedMeshCount == 1) {
        try {
            if (characterMesh) {
                walkingAnimation = std::make_unique<Animation>("assets/models/Character 1/Standing Idle.fbx", characterMesh.get());
                animator = std::make_unique<Animator>(walkingAnimation.get());
            } else {
                animator = std::make_unique<Animator>(nullptr);
            }
        } catch (const std::exception& e) {
            std::cerr << "[Warning] standing animation failed to load: " << e.what() << "\n";
            animator = std::make_unique<Animator>(nullptr);
        }
        m_loadedMeshCount = 2;
        return true;
    }
    return false;
}

bool Game::Initialize(Engine& engine) {
    if (!InitializePhase0(engine)) return false;
    if (!InitializePhase1(engine)) return false;
    if (!InitializePhase2(engine)) return false;
    while (LoadNextCharacterMesh()) {}
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
            title << " | " << interactionPrompt << " (G)";
        } else {
            title << " | " << interactionPrompt << " (E)";
        }
    }

    title << " | 1-5 switch | WASD move | F enter/exit house | E interact | Q abandon quest | Space jump | C crouch | Shift sprint";
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

    // ==========================================
    // 1. TOP-LEFT: ACTIVE CHARACTER STATUS (NIER-STYLE)
    // ==========================================
    if (activeChar) {
        // Name & Level flavor
        std::string nameStr = "UNIT: " + activeChar->GetName() + "  [LV 99]";
        hudRenderer->RenderText(nameStr, 30.0f, static_cast<float>(height) - 40.0f, 0.55f, glm::vec3(1.0f, 0.72f, 0.18f), width, height);

        // Procedural Nier-style Health Bar
        int hp = static_cast<int>(activeChar->GetHealth());
        int barWidth = 20;
        int filled = (hp * barWidth) / 100;
        std::string hpBar = "HP [";
        for (int i = 0; i < barWidth; ++i) {
            if (i < filled) hpBar += "I";
            else hpBar += ".";
        }
        hpBar += "]  " + std::to_string(hp) + "%";
        
        hudRenderer->RenderText(hpBar, 30.0f, static_cast<float>(height) - 62.0f, 0.50f, glm::vec3(0.95f, 0.96f, 0.98f), width, height);

        // Crouch/Sprint Indicators
        std::string stateStr = "STATE: ACTIVE";
        if (activeChar->IsCrouching()) {
            stateStr += "  [ • CROUCH ]";
        }
        if (activeChar->IsSprinting()) {
            stateStr += "  [ • SPRINT ]";
        }
        hudRenderer->RenderText(stateStr, 30.0f, static_cast<float>(height) - 82.0f, 0.42f, glm::vec3(0.6f, 0.65f, 0.7f), width, height);
    }

    // ==========================================
    // 2. TOP-RIGHT: WORLD ENVIRONMENT INFO
    // ==========================================
    if (questSystem) {
        std::string dayStr = "DAY 0" + std::to_string(static_cast<int>(questSystem->GetDayNumber()) + 1);
        std::string phaseStr = "PHASE: ";
        switch (questSystem->GetCurrentPhase()) {
            case StoryPhase::Exploration:   phaseStr += "EXPLORATION"; break;
            case StoryPhase::Revelation:    phaseStr += "REVELATION"; break;
            case StoryPhase::Confrontation: phaseStr += "CONFRONTATION"; break;
            case StoryPhase::Climax:        phaseStr += "CLIMAX"; break;
            case StoryPhase::Epilogue:      phaseStr += "EPILOGUE"; break;
        }

        std::string timeStr = world->IsNight() ? "TIME: NIGHTFALL [HAZARDOUS]" : "TIME: DAYLIGHT [SECURE]";
        glm::vec3 timeColor = world->IsNight() ? glm::vec3(1.0f, 0.2f, 0.2f) : glm::vec3(0.35f, 0.85f, 0.45f);

        float rightMargin = static_cast<float>(width) - 280.0f;
        hudRenderer->RenderText("♦  FROMVILLE ENGINE  ♦", rightMargin, static_cast<float>(height) - 40.0f, 0.48f, glm::vec3(1.0f, 0.72f, 0.18f), width, height);
        hudRenderer->RenderText(dayStr + "  |  " + phaseStr, rightMargin, static_cast<float>(height) - 62.0f, 0.44f, glm::vec3(0.75f, 0.76f, 0.78f), width, height);
        hudRenderer->RenderText(timeStr, rightMargin, static_cast<float>(height) - 82.0f, 0.42f, timeColor, width, height);

        // Sleek Right-Aligned Nier Visualizer Accent
        hudRenderer->RenderText("||.||..||..|||..||..||..||", rightMargin, static_cast<float>(height) - 98.0f, 0.38f, glm::vec3(0.35f, 0.38f, 0.42f), width, height);
    }

    // ==========================================
    // 3. MIDDLE CENTER: INTERACTION PROMPT
    // ==========================================
    if (!interactionPrompt.empty()) {
        const bool isPickup = world->NearestInteractionIsPickup();
        std::string keyPrompt = isPickup ? "[G] " : "[E] ";
        std::string fullPrompt = "▶  " + keyPrompt + "  " + interactionPrompt + "  ◀";
        
        float x = (static_cast<float>(width) / 2.0f) - (fullPrompt.length() * 10.0f * 0.65f / 2.0f);
        hudRenderer->RenderText(fullPrompt, x, 140.0f, 0.65f, glm::vec3(1.0f, 0.72f, 0.18f), width, height);
    }

    // ==========================================
    // 4. BOTTOM-LEFT: COMPACT QUEST LOG CARD
    // ==========================================
    std::vector<std::string> questLines;
    questLines.push_back("┌──  ACTIVE MISSION DATABASE  ──────────────");
    
    if (activeChar && questSystem) {
        const bool daytimeExplorationMode = !world->IsNight() &&
            (activeChar->GetType() == CharacterType::Jade || activeChar->GetType() == CharacterType::Tabitha);

        if (world->IsNight() && (activeChar->GetType() == CharacterType::Boyd || activeChar->GetType() == CharacterType::Victor)) {
            questLines.push_back("│  WARNING: Sunset has fallen! Get inside!");
            questLines.push_back("│  Quests paused while exposed to monsters.");
        } else if (daytimeExplorationMode) {
            questLines.push_back("│  MODE: Explore town and talk to NPCs.");
            questLines.push_back("│  Puzzle indicators will return at night.");
        } else {
            const Quest* quest = questSystem->GetCharacterQuest(activeChar->GetType());
            if (quest) {
                std::string title = quest->GetTitle();
                int progress = static_cast<int>(quest->GetProgress());
                questLines.push_back("│  QUEST: " + title + " [" + std::to_string(progress) + "%]");
                
                const auto& objectives = quest->GetObjectives();
                for (std::size_t objectiveIndex = 0; objectiveIndex < objectives.size(); ++objectiveIndex) {
                    if (!objectives[objectiveIndex].completed) {
                        questLines.push_back("│  NEXT:  " + objectives[objectiveIndex].description);
                        break;
                    }
                }
                
                const std::string waypointText = world->GetQuestWaypointText();
                if (!waypointText.empty()) {
                    questLines.push_back("│  LOC:   " + waypointText);
                }

                const std::string hint = quest->GetCurrentObjectiveHint(quest->GetNextIncompleteObjectiveIndex());
                if (!hint.empty()) {
                    questLines.push_back("│  TIP:   " + hint);
                }
            } else {
                questLines.push_back("│  QUEST: No active objective recorded.");
            }
        }
    }
    
    if (!questHelper.empty()) {
        questLines.push_back("│  HELP:  " + questHelper);
    }
    
    questLines.push_back("└───────────────────────────────────────────");

    float questStartY = static_cast<float>(height) - 130.0f;
    for (std::size_t i = 0; i < questLines.size(); ++i) {
        hudRenderer->RenderText(questLines[i], 30.0f, questStartY - (static_cast<float>(i) * 20.0f), 0.44f, glm::vec3(0.75f, 0.78f, 0.82f), width, height);
    }

    // ==========================================
    // 5. DRAMATIC WORLD EVENTS OVERLAYS
    // ==========================================
    if (world->GetNpcDialogueDisplayTime() > 0.0f) {
        const std::string& npcMsg = world->GetLastNpcDialogue();
        float x = (static_cast<float>(width) / 2.0f) - (npcMsg.length() * 8.0f * 0.7f / 2.0f);
        hudRenderer->RenderText(npcMsg, x, static_cast<float>(height) * 0.35f, 0.7f, glm::vec3(0.85f, 1.0f, 0.65f), width, height);
    }

    if (world->GetMonsterScreamDisplayTime() > 0.0f) {
        const std::string& screamMsg = world->GetLastMonsterScream();
        float x = (static_cast<float>(width) / 2.0f) - (screamMsg.length() * 10.0f * 0.9f / 2.0f);
        hudRenderer->RenderText(screamMsg, x, static_cast<float>(height) * 0.55f, 0.9f, glm::vec3(1.0f, 0.2f, 0.2f), width, height);
    }

    if (world->GetLastDamageDisplayTime() > 0.0f) {
        std::ostringstream damageMsg;
        damageMsg << "CRITICAL // DAMAGE RECEIVED -" << static_cast<int>(world->GetLastDamageAmount()) << " HP";
        std::string dStr = damageMsg.str();
        float x = (static_cast<float>(width) / 2.0f) - (dStr.length() * 9.0f * 0.75f / 2.0f);
        hudRenderer->RenderText(dStr, x, static_cast<float>(height) * 0.45f, 0.75f, glm::vec3(1.0f, 0.3f, 0.3f), width, height);
    }

    if (world->GetLastInteractionFeedbackTime() > 0.0f) {
        const std::string& feedbackMsg = world->GetLastInteractionFeedback();
        float x = (static_cast<float>(width) / 2.0f) - (feedbackMsg.length() * 8.0f * 0.75f / 2.0f);
        hudRenderer->RenderText(feedbackMsg, x, static_cast<float>(height) * 0.40f, 0.75f, glm::vec3(0.2f, 1.0f, 0.3f), width, height);
    }

    // ==========================================
    // 6. SYSTEM STATUS BAR (CONTROLS GUIDE)
    // ==========================================
    std::string sysControls = "SYS.CTRL // [1-4] SWITCH CHAR  [WASD] MOVE  [SPACE] JUMP  [C] CROUCH  [SHIFT] SPRINT  [E] INTERACT  [TAB] DIAGNOSTICS";
    hudRenderer->RenderText(sysControls, 30.0f, 30.0f, 0.38f, glm::vec3(0.55f, 0.58f, 0.62f), width, height);
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
        BeginCharacterSwitchTransition(0);
    } else if (input.IsActionPressed(InputAction::SwitchCharacter2)) {
        BeginCharacterSwitchTransition(1);
    } else if (input.IsActionPressed(InputAction::SwitchCharacter3)) {
        BeginCharacterSwitchTransition(2);
    } else if (input.IsActionPressed(InputAction::SwitchCharacter4)) {
        BeginCharacterSwitchTransition(3);
    }

    // Handle spawn restart requests triggered by other systems
    if (world->ConsumeSpawnRestartRequest()) {
        camera->Reset(spawnPosition);
        lastInteractionPrompt.clear();
        return;
    }
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

    const InputContext& input = engine.GetGameplayInput();
    Character* activeChar = world ? world->GetActiveCharacter() : nullptr;
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
        world->TryInteract();
    }
    if (input.IsActionPressed(InputAction::TalkOrQuest)) {
        world->TryActiveCharacterInteraction();
    }
    if (input.IsActionPressed(InputAction::Pickup)) {
        world->TryActiveCharacterPickup();
    }
    if (input.IsActionPressed(InputAction::Ability)) {
        world->TryActiveCharacterAbility();
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
        float targetAngle = glm::degrees(std::atan2(moveDir.x, moveDir.z));
        float currentAngle = activeChar->transform.rotation.y;
        float diff = targetAngle - currentAngle;
        while (diff < -180.0f) diff += 360.0f;
        while (diff > 180.0f) diff -= 360.0f;
        activeChar->transform.rotation.y = currentAngle + diff * glm::clamp(10.0f * dt, 0.0f, 1.0f);
    }
}

void Game::BeginCharacterSwitchTransition(int newIndex) {
    if (switchTransition.state != CharSwitchState::None) return; // already switching
    if (newIndex == world->GetActiveCharacterIndex()) return; // same character

    switchTransition.state = CharSwitchState::ZoomOut;
    switchTransition.timer = 0.0f;
    switchTransition.pendingCharacterIndex = newIndex;

    // Sweep will go from above current character to above new character
    glm::vec3 currentPos = world->GetActiveCharacter()->transform.position;
    glm::vec3 targetPos = world->GetCharacter(newIndex)->transform.position;
    switchTransition.sweepStartPos = currentPos + glm::vec3(0, switchTransition.sweepHeight, 0);
    switchTransition.sweepTargetPos = targetPos + glm::vec3(0, switchTransition.sweepHeight, 0);
}

void Game::UpdateCharacterSwitchTransition(float dt) {
    if (switchTransition.state == CharSwitchState::None) return;

    switchTransition.timer += dt;

    if (switchTransition.state == CharSwitchState::ZoomOut) {
        float t = switchTransition.timer / switchTransition.zoomOutDuration;
        t = glm::clamp(t, 0.0f, 1.0f);
        // Smoothly increase FOV and lift camera up
        camera->fovOverride = switchTransition.baseFov + switchTransition.sweepFovBoost * t;
        glm::vec3 activePos = world->GetActiveCharacter()->transform.position;
        camera->SetPositionOverride(activePos + glm::vec3(0, glm::mix(1.7f, switchTransition.sweepHeight, t), 0));

        if (switchTransition.timer >= switchTransition.zoomOutDuration) {
            switchTransition.state = CharSwitchState::Sweep;
            switchTransition.timer = 0.0f;
            // Now actually switch the character in world so we can target the new one
            world->SwitchCharacter(switchTransition.pendingCharacterIndex);
        }
    }
    else if (switchTransition.state == CharSwitchState::Sweep) {
        float t = switchTransition.timer / switchTransition.sweepDuration;
        t = glm::clamp(t, 0.0f, 1.0f);
        // Smooth step for cinematic feel
        float smoothT = t * t * (3.0f - 2.0f * t);
        glm::vec3 sweepPos = glm::mix(switchTransition.sweepStartPos, switchTransition.sweepTargetPos, smoothT);
        camera->SetPositionOverride(sweepPos);
        camera->fovOverride = switchTransition.baseFov + switchTransition.sweepFovBoost;

        if (switchTransition.timer >= switchTransition.sweepDuration) {
            switchTransition.state = CharSwitchState::ZoomIn;
            switchTransition.timer = 0.0f;
        }
    }
    else if (switchTransition.state == CharSwitchState::ZoomIn) {
        float t = switchTransition.timer / switchTransition.zoomInDuration;
        t = glm::clamp(t, 0.0f, 1.0f);
        float smoothT = t * t * (3.0f - 2.0f * t);
        // Bring camera back down to eye level on the new character
        glm::vec3 newCharPos = world->GetActiveCharacter()->transform.position;
        glm::vec3 fromPos = switchTransition.sweepTargetPos;
        glm::vec3 toPos = newCharPos + glm::vec3(0, 1.7f, 0);
        camera->SetPositionOverride(glm::mix(fromPos, toPos, smoothT));
        camera->fovOverride = (switchTransition.baseFov + switchTransition.sweepFovBoost) - switchTransition.sweepFovBoost * smoothT;

        if (switchTransition.timer >= switchTransition.zoomInDuration) {
            // Transition complete
            switchTransition.state = CharSwitchState::None;
            camera->fovOverride = -1.0f;
            camera->ClearPositionOverride();
        }
    }
}

void Game::Update(float dt, Engine& engine) {
    if (!camera || !world) {
        return;
    }

    if (loadState == GameLoadState::LoadingCharacters) {
        bool moreToLoad = world->LoadNextPendingMesh();
        if (!moreToLoad) {
            loadState = GameLoadState::StoryIntro;
            introElapsed = 0.0f;
            std::cout << "[LoadState] Characters fully loaded. Presenting Story Intro Screen.\n";
            if (AudioManager* audio = world->GetAudioManager()) {
                audio->PlaySound("intro_music", 0.65f);
            }
        }
        dayNightCycle.syncToWorldClock(world->GetWorldClock());
        return;
    }

    if (loadState == GameLoadState::StoryIntro) {
        introElapsed += dt;
        if (engine.GetInput().IsKeyPressed(GLFW_KEY_ENTER) || engine.GetInput().IsKeyPressed(GLFW_KEY_SPACE)) {
            loadState = GameLoadState::MainMenu;
            std::cout << "[LoadState] Story Intro complete. Entering Main Menu.\n";
        }
        return;
    }

    if (loadState == GameLoadState::MainMenu) {
        const bool hasSave = std::filesystem::exists("savegame.txt");
        if (!hasSave) {
            selectedMenuIndex = 0; // Force New Game if no save exists
        }

        // Keyboard navigation
        if (engine.GetInput().IsKeyPressed(GLFW_KEY_UP) || engine.GetInput().IsKeyPressed(GLFW_KEY_W)) {
            selectedMenuIndex = 0; // New Game
        }
        if (engine.GetInput().IsKeyPressed(GLFW_KEY_DOWN) || engine.GetInput().IsKeyPressed(GLFW_KEY_S)) {
            if (hasSave) {
                selectedMenuIndex = 1; // Continue
            }
        }

        // Confirmation selection
        if (engine.GetInput().IsKeyPressed(GLFW_KEY_ENTER) || engine.GetInput().IsKeyPressed(GLFW_KEY_SPACE)) {
            if (AudioManager* audio = world->GetAudioManager()) {
                audio->StopSound("intro_music");
                audio->PlaySound("ambient_tension_low", 0.30f);
            }

            if (selectedMenuIndex == 0) {
                // START FRESH NEW GAME
                loadState = GameLoadState::Ready;
                std::cout << "[LoadState] Starting NEW GAME fresh.\n";
            } else if (selectedMenuIndex == 1 && hasSave) {
                // RESUME FROM SAVED FILE
                if (world->LoadFromFile("savegame.txt")) {
                    if (Character* activeChar = world->GetActiveCharacter()) {
                        camera->Reset(activeChar->transform.position);
                    }
                    dayNightCycle.syncToWorldClock(world->GetWorldClock());
                    std::cout << "[LoadState] Resumed game from savegame.txt successfully.\n";
                }
                loadState = GameLoadState::Ready;
            }
        }
        return;
    }
    // Debug teleport: press F9 to jump to the colony house for verification
    if (engine.GetInput().IsKeyPressed(GLFW_KEY_F9)) {
        glm::vec3 debugCam = glm::vec3(9.0f, 1.8f, 8.0f);
        camera->Reset(debugCam);
        if (Character* ac = world->GetActiveCharacter()) {
            ac->transform.position = glm::vec3(9.0f, 0.0f, 8.0f);
            ac->transform.rotation.y = 0.0f;
        }
    }

    // Debug fast-forward time: F10 advances world clock to trigger day/night transitions
    if (engine.GetInput().IsKeyPressed(GLFW_KEY_F10)) {
        if (world) {
            // Advance by 70 seconds which crosses the sunset boundary in the 120s cycle
            world->AdvanceWorldClock(70.0f);
        }
    }

    if (switchTransition.state == CharSwitchState::None) {
        HandleGlobalInput(engine);
        HandleCharacterInput(dt, engine);

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
    }

    UpdateCharacterSwitchTransition(dt);

    if (camera) {
        if (Character* activeChar = world->GetActiveCharacter()) {
            camera->Update(engine.GetInput(), dt, activeChar->transform.position);
        }
    }

    const std::string interactionPrompt = world->GetInteractionPrompt();
    if (interactionPrompt != lastInteractionPrompt) {
        lastInteractionPrompt = interactionPrompt;
        if (!interactionPrompt.empty()) {
            std::cout << "[Interact] " << interactionPrompt << " (E)\n";
        }
    }

    world->Update(*camera, dt);
    // Apply any pending debug advance to world clock
    if (pendingAdvanceSeconds > 0.0f && world) {
        world->AdvanceWorldClock(pendingAdvanceSeconds);
        pendingAdvanceSeconds = 0.0f;
    }
    
    // Keep the visual cycle locked to the world clock so gameplay and sky flip together.
    dayNightCycle.syncToWorldClock(world->GetWorldClock());

    if (animator) {
        animator->UpdateAnimation(dt);
    }
}

void Game::Render(Engine& engine) const {
    if (!camera || !world) {
        return;
    }

    if (loadState == GameLoadState::LoadingCharacters || loadState == GameLoadState::StoryIntro || loadState == GameLoadState::MainMenu) {
        int width = engine.GetWindow().GetWidth();
        int height = engine.GetWindow().GetHeight();
        
        // Keep screen background dark and atmospheric so the floating parchment paper pops out beautifully!
        glClearColor(0.04f, 0.05f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (hudRenderer) {
            if (loadState == GameLoadState::LoadingCharacters) {
                std::string title = "FROMVILLE";
                float titleScale = 1.6f;
                float titleX = (static_cast<float>(width) / 2.0f) - (title.length() * 14.0f * titleScale / 2.0f);
                hudRenderer->RenderText(title, titleX, static_cast<float>(height) / 2.0f + 40.0f, titleScale, glm::vec3(0.95f, 0.96f, 0.98f), width, height);

                std::string status = "DECOMPRESSING 3D ASSETS & ANIMATIONS...";
                float statusScale = 0.5f;
                float statusX = (static_cast<float>(width) / 2.0f) - (status.length() * 10.0f * statusScale / 2.0f);
                glm::vec3 statusColor = glm::vec3(0.45f, 0.65f, 0.95f);
                hudRenderer->RenderText(status, statusX, static_cast<float>(height) / 2.0f - 20.0f, statusScale, statusColor, width, height);

                std::string prompt = "PLEASE STAND BY";
                glm::vec3 promptColor = glm::vec3(0.5f, 0.55f, 0.65f);
                float promptScale = 0.4f;
                float promptX = (static_cast<float>(width) / 2.0f) - (prompt.length() * 10.0f * promptScale / 2.0f);
                hudRenderer->RenderText(prompt, promptX, static_cast<float>(height) / 2.0f - 80.0f, promptScale, promptColor, width, height);
            } else if (loadState == GameLoadState::StoryIntro) {
                // RENDER PREMIUM ATMOSPHERIC CINEMATIC STORY INTRO WITH SEQUENTIAL FADE-INS
                auto getFadeAlpha = [this](float startTime) -> float {
                    if (introElapsed < startTime) return 0.0f;
                    return std::min(1.0f, (introElapsed - startTime) / 1.5f); // 1.5s fade-in duration
                };

                // Pulse factor for elements that draw attention
                float pulse = 0.7f + 0.3f * std::sin(world->GetWorldClock() * 4.0f);

                // --- ULTRA-SMOOTH CONTINUOUS INTERPOLATION ---
                // Transition phase starts at 5.5s (after warning fades out completely) and finishes unfolding at 7.0s (1.5s duration)
                float t_smooth = 0.0f;
                if (introElapsed >= 5.5f) {
                    float t_raw = std::min(1.0f, (introElapsed - 5.5f) / 1.5f);
                    t_smooth = t_raw * t_raw * (3.0f - 2.0f * t_raw); // smoothstep s-curve
                }

                // Settle float factor (floating motion dampens down as paper unfolds)
                float floatScale = 1.0f - t_smooth;
                float floatX = std::cos(introElapsed * 2.2f) * 20.0f * floatScale;
                float floatY = std::sin(introElapsed * 2.8f) * 25.0f * floatScale;

                float startW = 600.0f;
                float startH = 160.0f;
                float targetW = static_cast<float>(width) - 100.0f;
                float targetH = static_cast<float>(height) - 80.0f;

                float curW = startW + t_smooth * (targetW - startW);
                float curH = startH + t_smooth * (targetH - startH);

                float curX = (static_cast<float>(width) / 2.0f) - (curW / 2.0f) + floatX;
                float curY = (static_cast<float>(height) / 2.0f) - (curH / 2.0f) + floatY;

                // Fades for overall background / card (starts at 0.0, reaches 1.0 at 1.5 seconds)
                float paperAlpha = std::min(1.0f, introElapsed / 1.5f);

                glm::vec3 darkShadow = glm::vec3(0.12f, 0.09f, 0.07f) * paperAlpha; // card shadow
                glm::vec3 paperColor = glm::vec3(0.88f, 0.83f, 0.73f) * paperAlpha; // warm parchment beige

                // --- PROCEDURAL TORN & WORN PAPER RENDERER ---
                // Slices the paper horizontally and adds jagged sine/cosine wave coordinate offsets
                // and authentic tears to make the parchment look weatherworn and ragged.
                auto drawTornPaper = [&](float px, float py, float pw, float ph, const glm::vec3& pcolor) {
                    const int N = 32;
                    float sliceHeight = ph / static_cast<float>(N);
                    
                    for (int i = 0; i < N; ++i) {
                        float sliceY = py + static_cast<float>(i) * sliceHeight;
                        
                        // Dynamic organic edge jaggedness
                        float edgeOffsetL = std::sin(static_cast<float>(i) * 0.9f) * 4.0f 
                                          + std::cos(static_cast<float>(i) * 2.3f) * 2.0f;
                                          
                        float edgeOffsetR = std::cos(static_cast<float>(i) * 1.1f) * 4.0f 
                                          + std::sin(static_cast<float>(i) * 2.7f) * 2.0f;
                        
                        // Beautiful worn-down rips at specific vertical places
                        float tearIn = 0.0f;
                        if (i == 4)  tearIn = 16.0f;  // deep tear left
                        if (i == 14) tearIn = -14.0f; // deep tear right
                        if (i == 24) tearIn = 18.0f;  // deep tear left
                        
                        float finalSliceX = px + edgeOffsetL + (tearIn > 0.0f ? tearIn : 0.0f);
                        float finalSliceW = pw + edgeOffsetR - edgeOffsetL - std::abs(tearIn);
                        
                        // Notch top and bottom edges slightly to make them ragged
                        float finalSliceH = sliceHeight + 1.5f;
                        if (i == 0 || i == N - 1) {
                            finalSliceH -= 2.0f;
                        }
                        
                        hudRenderer->RenderQuad(finalSliceX, sliceY, finalSliceW, finalSliceH, pcolor, width, height);
                    }
                };

                // 1. Render Floating Paper Plane drop shadow
                drawTornPaper(curX - 4.0f, curY - 4.0f, curW + 8.0f, curH + 8.0f, darkShadow);
                // 2. Render Floating Paper Plane primary parchment sheet
                drawTornPaper(curX, curY, curW, curH, paperColor);

                // --- STAGE 2 TEXT (context story, objectives, character backgrounds) ---
                // Fades in sequentially starting at 7.0s (when paper is fully unfolded)
                if (t_smooth > 0.0f) {
                    // Decorative Section Separator template
                    std::string separator = "~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~";
                    float sepScale = 0.45f;
                    float sepX = curX + (curW / 2.0f) - (separator.length() * 10.0f * sepScale / 2.0f);

                    // Title (Dark crimson blood ink)
                    std::string title = "♦  FROMVILLE: THE NIGHTMARE BEGINS  ♦";
                    float titleScale = 1.3f;
                    float titleX = curX + (curW / 2.0f) - (title.length() * 14.0f * titleScale / 2.0f);
                    hudRenderer->RenderText(title, titleX, curY + curH - 50.0f, titleScale, glm::vec3(0.75f, 0.10f, 0.10f), width, height, getFadeAlpha(7.0f));

                    // --- 1. THE STORY BACKGROUND (Sienna brown and walnut ink) ---
                    std::string storyHeader = "THE MYSTERY & BACKGROUND";
                    hudRenderer->RenderText(storyHeader, curX + 50.0f, curY + curH - 110.0f, 0.55f, glm::vec3(0.40f, 0.20f, 0.05f), width, height, getFadeAlpha(8.0f));

                    std::string storyLine1 = "You are trapped in a mysterious town in middle America that traps everyone who enters.";
                    std::string storyLine2 = "The roads loop infinitely back to town. The forest is thick, dark, and alive with ancient power.";
                    std::string storyLine3 = "And at night, nightmarish monsters crawl out of the woods. They don't run, they don't hide...";
                    std::string storyLine4 = "They walk calmly. They smile. And if you let them in, they will tear you apart.";
                    
                    hudRenderer->RenderText(storyLine1, curX + 50.0f, curY + curH - 145.0f, 0.42f, glm::vec3(0.18f, 0.15f, 0.12f), width, height, getFadeAlpha(9.0f));
                    hudRenderer->RenderText(storyLine2, curX + 50.0f, curY + curH - 170.0f, 0.42f, glm::vec3(0.18f, 0.15f, 0.12f), width, height, getFadeAlpha(10.0f));
                    hudRenderer->RenderText(storyLine3, curX + 50.0f, curY + curH - 195.0f, 0.42f, glm::vec3(0.18f, 0.15f, 0.12f), width, height, getFadeAlpha(11.0f));
                    hudRenderer->RenderText(storyLine4, curX + 50.0f, curY + curH - 220.0f, 0.42f, glm::vec3(0.18f, 0.15f, 0.12f), width, height, getFadeAlpha(12.0f));

                    // First Separator
                    hudRenderer->RenderText(separator, sepX, curY + curH - 250.0f, sepScale, glm::vec3(0.60f, 0.52f, 0.42f), width, height, getFadeAlpha(13.0f));

                    // --- 2. YOUR MISSIONS (Sienna brown and Prussian blue ink) ---
                    std::string missionsHeader = "YOUR SURVIVAL OBJECTIVES";
                    hudRenderer->RenderText(missionsHeader, curX + 50.0f, curY + curH - 280.0f, 0.55f, glm::vec3(0.40f, 0.20f, 0.05f), width, height, getFadeAlpha(13.5f));

                    std::string mission1 = "• SURVIVE THE NOCTURNAL HUNT: Secure safety inside houses before the sun sets.";
                    std::string mission2 = "• PUZZLE SOLVING: Only Tabitha and Jade can decipher ancient talisman puzzles at night.";
                    std::string mission3 = "• MAINTAIN COMMUNITY: Watch over the townspeople who roam doing chores during the day.";

                    hudRenderer->RenderText(mission1, curX + 50.0f, curY + curH - 315.0f, 0.42f, glm::vec3(0.10f, 0.22f, 0.38f), width, height, getFadeAlpha(14.5f));
                    hudRenderer->RenderText(mission2, curX + 50.0f, curY + curH - 340.0f, 0.42f, glm::vec3(0.10f, 0.22f, 0.38f), width, height, getFadeAlpha(15.5f));
                    hudRenderer->RenderText(mission3, curX + 50.0f, curY + curH - 365.0f, 0.42f, glm::vec3(0.10f, 0.22f, 0.38f), width, height, getFadeAlpha(16.5f));

                    // Second Separator
                    hudRenderer->RenderText(separator, sepX, curY + curH - 395.0f, sepScale, glm::vec3(0.60f, 0.52f, 0.42f), width, height, getFadeAlpha(18.0f));

                    // --- 3. CHARACTER BACKGROUNDS (Sienna brown and faded iron-gall ink) ---
                    std::string charHeader = "PLAYABLE SURVIVOR ROLES";
                    hudRenderer->RenderText(charHeader, curX + 50.0f, curY + curH - 430.0f, 0.55f, glm::vec3(0.40f, 0.20f, 0.05f), width, height, getFadeAlpha(18.5f));

                    std::string charBoyd    = "• SHERIFF BOYD: The weary leader keeping order. Spawns with 100% resolve.";
                    std::string charJade    = "• JADE HERERA: Brilliant, arrogant tech-mogul. Deciphers mathematical stones.";
                    std::string charTabitha = "• TABITHA MATTHEWS: Determined mother looking for her child and the lighthouse exit.";
                    std::string charVictor  = "• VICTOR: Mysterious artist who survived here for decades. Knows hidden paths.";

                    hudRenderer->RenderText(charBoyd, curX + 50.0f, curY + curH - 465.0f, 0.42f, glm::vec3(0.20f, 0.24f, 0.28f), width, height, getFadeAlpha(19.5f));
                    hudRenderer->RenderText(charJade, curX + 50.0f, curY + curH - 490.0f, 0.42f, glm::vec3(0.20f, 0.24f, 0.28f), width, height, getFadeAlpha(20.5f));
                    hudRenderer->RenderText(charTabitha, curX + 50.0f, curY + curH - 515.0f, 0.42f, glm::vec3(0.20f, 0.24f, 0.28f), width, height, getFadeAlpha(21.5f));
                    hudRenderer->RenderText(charVictor, curX + 50.0f, curY + curH - 540.0f, 0.42f, glm::vec3(0.20f, 0.24f, 0.28f), width, height, getFadeAlpha(22.5f));
                }

                // --- STAGE 1 TEXT (bloody typewriter warning and stamp subtitle) ---
                // Appears ONLY after the paper card (plain) is fully visible (starts at 1.5s)
                // Fades out completely from 4.5s to 5.5s
                if (introElapsed >= 1.5f && introElapsed < 5.5f) {
                    float warningAlpha = 1.0f;
                    if (introElapsed >= 4.5f) {
                        warningAlpha = std::max(0.0f, 1.0f - (introElapsed - 4.5f) / 1.0f); // 1.0s fade out
                    }

                    glm::vec3 phraseColor = glm::vec3(0.75f, 0.10f, 0.10f); 
                    glm::vec3 stampColor = glm::vec3(0.40f, 0.30f, 0.15f);

                    std::string phrase = "KNOWLEDGE COMES AT A COST...";
                    float typingSpeed = 14.0f;
                    float typeElapsed = introElapsed - 1.5f; // Offset typewriter start time!
                    int charsToShow = std::min(static_cast<int>(phrase.length()), static_cast<int>(typeElapsed * typingSpeed));
                    std::string typedPhrase = phrase.substr(0, charsToShow);
                    
                    if (charsToShow < static_cast<int>(phrase.length()) && (static_cast<int>(typeElapsed * 4.0f) % 2 == 0)) {
                        typedPhrase += "_";
                    }

                    float pScale = 0.55f;
                    float pX = curX + (curW / 2.0f) - (phrase.length() * 10.0f * pScale / 2.0f);
                    float pY = curY + (curH / 2.0f) + 15.0f;

                    hudRenderer->RenderText(typedPhrase, pX, pY, pScale, phraseColor, width, height, warningAlpha);

                    std::string subText = "--- Journal Entry #1978 ---";
                    float sScale = 0.40f;
                    float sX = curX + (curW / 2.0f) - (subText.length() * 10.0f * sScale / 2.0f);
                    float sY = curY + (curH / 2.0f) - 25.0f;
                    
                    hudRenderer->RenderText(subText, sX, sY, sScale, stampColor, width, height, warningAlpha);
                }

                // --- 4. PROMPT TO CONTINUE (VISIBLE AFTER CARD FADES IN - Rich Amber ink) ---
                std::string prompt = "PRESS [SPACE] OR [ENTER] TO ACCESS MAIN MENU";
                float promptScale = 0.55f;
                float promptX = (static_cast<float>(width) / 2.0f) - (prompt.length() * 10.0f * promptScale / 2.0f);
                
                glm::vec3 promptBaseColor = (introElapsed < 4.0f) ? glm::vec3(1.0f, 0.72f, 0.18f) : glm::vec3(0.45f, 0.22f, 0.05f);
                float promptAlpha = getFadeAlpha(1.8f);
                hudRenderer->RenderText(prompt, promptX, 50.0f, promptScale, promptBaseColor, width, height, promptAlpha * pulse);
            } else if (loadState == GameLoadState::MainMenu) {
                // RENDER PREMIUM MAIN MENU
                std::string title = "♦  FROMVILLE  ♦";
                float titleScale = 2.0f;
                float titleX = (static_cast<float>(width) / 2.0f) - (title.length() * 14.0f * titleScale / 2.0f);
                hudRenderer->RenderText(title, titleX, static_cast<float>(height) / 2.0f + 140.0f, titleScale, glm::vec3(1.0f, 0.72f, 0.18f), width, height);

                std::string subtitle = "NO WAY OUT";
                float subtitleScale = 0.8f;
                float subtitleX = (static_cast<float>(width) / 2.0f) - (subtitle.length() * 10.0f * subtitleScale / 2.0f);
                hudRenderer->RenderText(subtitle, subtitleX, static_cast<float>(height) / 2.0f + 85.0f, subtitleScale, glm::vec3(0.75f, 0.76f, 0.78f), width, height);

                std::string divider = "─────────────────────────────────────────";
                float divScale = 0.5f;
                float divX = (static_cast<float>(width) / 2.0f) - (divider.length() * 10.0f * divScale / 2.0f);
                hudRenderer->RenderText(divider, divX, static_cast<float>(height) / 2.0f + 50.0f, divScale, glm::vec3(0.35f, 0.38f, 0.42f), width, height);

                const bool hasSave = std::filesystem::exists("savegame.txt");

                // 1. New Game button
                std::string newGameText = (selectedMenuIndex == 0) ? "▶  ♦ NEW GAME ♦  ◀" : "   NEW GAME   ";
                glm::vec3 newGameColor = (selectedMenuIndex == 0) ? glm::vec3(1.0f, 0.72f, 0.18f) : glm::vec3(0.5f, 0.55f, 0.6f);
                float newGameScale = (selectedMenuIndex == 0) ? 0.8f : 0.6f;
                float newGameX = (static_cast<float>(width) / 2.0f) - (newGameText.length() * 10.0f * newGameScale / 2.0f);
                hudRenderer->RenderText(newGameText, newGameX, static_cast<float>(height) / 2.0f - 20.0f, newGameScale, newGameColor, width, height);

                // 2. Continue button
                std::string continueText = "   CONTINUE   ";
                glm::vec3 continueColor = glm::vec3(0.5f, 0.55f, 0.6f);
                if (!hasSave) {
                    continueText = "   CONTINUE (NO SAVE)   ";
                    continueColor = glm::vec3(0.22f, 0.24f, 0.26f); // disabled color
                } else if (selectedMenuIndex == 1) {
                    continueText = "▶  ♦ CONTINUE ♦  ◀";
                    continueColor = glm::vec3(1.0f, 0.72f, 0.18f);
                }
                float continueScale = (selectedMenuIndex == 1) ? 0.8f : 0.6f;
                float continueX = (static_cast<float>(width) / 2.0f) - (continueText.length() * 10.0f * continueScale / 2.0f);
                hudRenderer->RenderText(continueText, continueX, static_cast<float>(height) / 2.0f - 90.0f, continueScale, continueColor, width, height);

                // Navigation Controls instruction
                std::string controls = "[W / S] Navigate  |  [ENTER] Confirm Selection";
                float controlsScale = 0.4f;
                float controlsX = (static_cast<float>(width) / 2.0f) - (controls.length() * 10.0f * controlsScale / 2.0f);
                hudRenderer->RenderText(controls, controlsX, static_cast<float>(height) / 2.0f - 180.0f, controlsScale, glm::vec3(0.4f, 0.45f, 0.5f), width, height);
            }
        }
        return;
    }

    const float aspectRatio = engine.GetAspectRatio();
    const glm::mat4 view = camera->GetViewMatrix();
    const glm::mat4 projection = camera->GetProjectionMatrix(aspectRatio);
    const glm::vec3 cameraPos = camera->GetPosition();
    const float currentTime = static_cast<float>(glfwGetTime());
    const float fogDensity = 0.018f;

    // 1. Skydome
    skydomeRenderer.render(view, projection,
                           dayNightCycle.getSunDirection(),
                           dayNightCycle.getDayFactor(),
                           dayNightCycle.getDayTime());

    // 2. Ground quad
    groundRenderer.render(view, projection, cameraPos,
                          dayNightCycle.getActiveLightDir(),
                          dayNightCycle.getLightColor(),
                          dayNightCycle.getAmbientColor(),
                          dayNightCycle.getDiffuseStrength(),
                          dayNightCycle.getFogColor(), fogDensity);

    // 3. Grass
    grassRenderer.render(view, projection, cameraPos, currentTime,
                         dayNightCycle.getActiveLightDir(),
                         dayNightCycle.getLightColor(),
                         dayNightCycle.getAmbientColor(),
                         dayNightCycle.getDiffuseStrength(),
                         dayNightCycle.getFogColor(), fogDensity);

    // 3.5 Trees
    treeRenderer.render(view, projection, cameraPos,
                        dayNightCycle.getActiveLightDir(),
                        dayNightCycle.getLightColor(),
                        dayNightCycle.getAmbientColor(),
                        dayNightCycle.getDiffuseStrength(),
                        dayNightCycle.getFogColor(), fogDensity);

    // 4. World rendering (buildings, NPCs, entities, interaction overlays)
    world->RenderObjects(*camera, aspectRatio, dayNightCycle, fogDensity);
    world->Render(*camera, aspectRatio, dayNightCycle, fogDensity);

    // 5. Render animated character (active character)
    // Never render the active character mesh in first person view
    // (the character IS the camera in first person)
    // Only render if you want a body/shadow — skip entirely for now
    if (false) {
        if (characterMesh && animatedShader) {
            animatedShader->Bind();
            animatedShader->SetMat4("view", view);
            animatedShader->SetMat4("projection", projection);
            animatedShader->SetVec3("lightDir", dayNightCycle.getActiveLightDir());
            animatedShader->SetVec3("lightColor", dayNightCycle.getLightColor());
            animatedShader->SetVec3("viewPos", cameraPos);
            animatedShader->SetFloat("fogDensity", fogDensity);
            animatedShader->SetVec3("fogColor", dayNightCycle.getFogColor());

            if (Character* activeChar = world->GetActiveCharacter()) {
                glm::vec3 playerPos = activeChar->transform.position;
                float playerRot = activeChar->transform.rotation.y;

                glm::mat4 model = glm::translate(glm::mat4(1.0f), playerPos);
                model = glm::rotate(model, glm::radians(playerRot), glm::vec3(0.0f, 1.0f, 0.0f));
                model = glm::scale(model, glm::vec3(0.01f)); 
                animatedShader->SetMat4("model", model);

                if (animator) {
                    animatedShader->SetMat4Array("finalBonesMatrices", animator->GetFinalBoneMatrices());
                }

                characterMesh->draw(animatedShader->GetID());
            }
        }
    }

    // 6. HUD rendering
    if (hudRenderer) {
        if (!world->IsPuzzleActive()) {
            const bool showDiagnostics = engine.GetInput().IsKeyDown(GLFW_KEY_TAB);
            world->RenderNarrativeOverlays(*hudRenderer, engine.GetWindow().GetWidth(), engine.GetWindow().GetHeight(), showDiagnostics);
        }
        world->RenderPuzzleOverlay(*hudRenderer, engine.GetWindow().GetWidth(), engine.GetWindow().GetHeight());
    }
    RenderHud(engine);
}

void Game::Shutdown() {
    groundRenderer.cleanup();
    grassRenderer.cleanup();
    treeRenderer.cleanup();
    skydomeRenderer.cleanup();

    hudRenderer.reset();
    world.reset();
    camera.reset();
    characterMesh.reset();
    animatedShader.reset();
}

void Game::RequestAdvanceWorldClock(float seconds) {
    pendingAdvanceSeconds = seconds;
}

void Game::ApplyDifficulty(Difficulty difficulty) {
    if (world) {
        world->ApplyDifficulty(difficulty);
    }
}

bool Game::HasAnyCharacterDied() const {
    return world ? world->HasAnyCharacterDied() : false;
}

std::string Game::GetLastDeadCharacterName() const {
    return world ? world->GetLastDeadCharacterName() : "";
}

std::string Game::GetDayTimeString() const {
    if (!world) return "";
    float clock = world->GetWorldClock();
    float hour24 = 6.0f + clock;
    if (hour24 >= 24.0f) hour24 -= 24.0f;
    int hours = static_cast<int>(hour24);
    int minutes = static_cast<int>((hour24 - hours) * 60.0f);
    char buf[32];
    std::sprintf(buf, "%02d:%02d", hours, minutes);
    return std::string(buf) + (world->IsNight() ? " [NIGHT]" : " [DAY]");
}

bool Game::SaveGame() const {
    return world ? world->SaveToFile("savegame.txt") : false;
}

bool Game::LoadGame() {
    return world ? world->LoadFromFile("savegame.txt") : false;
}

void Game::SetCursorLocked(bool locked) {
    cursorLocked = locked;
}
