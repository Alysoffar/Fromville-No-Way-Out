#include "game/ui/MenuSystem.h"
#include "engine/core/Engine.h"
#include "engine/renderer/TextRenderer.h"
#include "game/Game.h"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <algorithm>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

bool MenuSystem::Initialize(Engine& engine, int screenWidth, int screenHeight) {
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;
    m_engine = &engine;

    // Create and initialize the text renderer
    m_textRenderer = std::make_unique<TextRenderer>();
    if (!m_textRenderer->Initialize("", 24)) {
        std::cerr << "[MenuSystem] Failed to initialize TextRenderer.\n";
    }

    // Set TextRenderer on UIRenderer BEFORE initializing it
    m_uiRenderer.SetTextRenderer(m_textRenderer.get());

    if (!m_uiRenderer.Initialize(screenWidth, screenHeight)) {
        std::cerr << "[MenuSystem] Failed to initialize UIRenderer.\n";
        return false;
    }

    // Initialize MenuSystem independent AudioManager
    m_audioManager = std::make_unique<AudioManager>();
    if (m_audioManager->Initialize()) {
        m_audioManager->LoadSound("intro_music", "assets/audio/music/The Pixies - Que Sera_ Sera (Whatever Will Be_ Will Be) - Lyrics_ Audio quality.wav");
    }

    // Start with the story introduction first!
    m_game = nullptr;
    m_loadPhase = 0;
    m_loadingDisplayTimer = 0.0f;

    SetState(AppState::StoryIntro);
    return true;
}

void MenuSystem::Shutdown() {
    m_uiRenderer.Shutdown();
    m_textRenderer.reset();
    m_audioManager.reset();
    if (m_game) {
        delete m_game;
        m_game = nullptr;
    }
}

void MenuSystem::Update(float dt, Engine& engine) {
    // Check if window resized
    int w = engine.GetWindow().GetWidth();
    int h = engine.GetWindow().GetHeight();
    if (w != m_screenWidth || h != m_screenHeight) {
        m_screenWidth = w;
        m_screenHeight = h;
        m_uiRenderer.SetScreenSize(w, h);
    }

    if (m_state == AppState::StoryIntro) {
        UpdateStoryIntro(dt, engine);
        return;
    }

    if (m_state == AppState::Playing) {
        UpdatePlaying(dt, engine);
        return;
    }

    if (m_state == AppState::Loading) {
        UpdateLoading(dt, engine);
        return;
    }

    if (m_state == AppState::Paused) {
        if (m_pauseInputCooldown > 0.0f) {
            m_pauseInputCooldown -= dt;
        }
        if (m_pauseInputCooldown <= 0.0f && engine.GetGameplayInput().IsActionPressed(InputAction::Pause)) {
            SetState(AppState::Playing);
            m_pauseInputCooldown = 0.3f;
            if (m_game) {
                m_game->SetCursorLocked(true);
            }
            engine.GetInput().SetCursorLocked(true);
            return;
        }
    }

    // Settings volume click detector
    if (m_state == AppState::Settings) {
        if (engine.GetInput().IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
            double mx = 0.0, my = 0.0;
            glfwGetCursorPos(engine.GetWindow().GetHandle(), &mx, &my);
            float normX = static_cast<float>(mx) / m_screenWidth;
            float normY = static_cast<float>(my) / m_screenHeight;

            // Volume bracket detection at y = 0.62f
            if (std::abs(normY - 0.62f) < 0.04f) {
                if (normX >= 0.38f && normX <= 0.46f) {
                    m_masterVolume = std::max(0.0f, m_masterVolume - 0.1f);
                    m_settingsChanged = true;
                    if (m_game && m_game->GetWorld() && m_game->GetWorld()->GetAudioManager()) {
                        m_game->GetWorld()->GetAudioManager()->SetMasterVolume(m_masterVolume);
                    }
                } else if (normX >= 0.54f && normX <= 0.62f) {
                    m_masterVolume = std::min(1.0f, m_masterVolume + 0.1f);
                    m_settingsChanged = true;
                    if (m_game && m_game->GetWorld() && m_game->GetWorld()->GetAudioManager()) {
                        m_game->GetWorld()->GetAudioManager()->SetMasterVolume(m_masterVolume);
                    }
                }
            }
        }
    }

    // 1. Gather visible buttons
    auto& buttons = m_uiRenderer.GetButtons();
    std::vector<UIButton*> visibleButtons;
    for (auto& btn : buttons) {
        if (btn.visible) {
            visibleButtons.push_back(&btn);
        }
    }

    // 2. Poll keys for selection and adjustment
    bool upPressed = engine.GetInput().IsKeyPressed(GLFW_KEY_UP) || engine.GetInput().IsKeyPressed(GLFW_KEY_W);
    bool downPressed = engine.GetInput().IsKeyPressed(GLFW_KEY_DOWN) || engine.GetInput().IsKeyPressed(GLFW_KEY_S);
    bool leftPressed = engine.GetInput().IsKeyPressed(GLFW_KEY_LEFT) || engine.GetInput().IsKeyPressed(GLFW_KEY_A);
    bool rightPressed = engine.GetInput().IsKeyPressed(GLFW_KEY_RIGHT) || engine.GetInput().IsKeyPressed(GLFW_KEY_D);
    bool confirmPressed = engine.GetInput().IsKeyPressed(GLFW_KEY_ENTER) || engine.GetInput().IsKeyPressed(GLFW_KEY_SPACE) || engine.GetInput().IsKeyPressed(GLFW_KEY_E);

    // Settings screen key volume adjustment
    if (m_state == AppState::Settings) {
        if (leftPressed) {
            m_masterVolume = std::max(0.0f, m_masterVolume - 0.1f);
            m_settingsChanged = true;
            if (m_game && m_game->GetWorld() && m_game->GetWorld()->GetAudioManager()) {
                m_game->GetWorld()->GetAudioManager()->SetMasterVolume(m_masterVolume);
            }
        } else if (rightPressed) {
            m_masterVolume = std::min(1.0f, m_masterVolume + 0.1f);
            m_settingsChanged = true;
            if (m_game && m_game->GetWorld() && m_game->GetWorld()->GetAudioManager()) {
                m_game->GetWorld()->GetAudioManager()->SetMasterVolume(m_masterVolume);
            }
        }
    }

    // 3. Handle Keyboard button navigation and confirmation
    if (!visibleButtons.empty()) {
        if (m_selectedButtonIndex < 0 || m_selectedButtonIndex >= static_cast<int>(visibleButtons.size())) {
            m_selectedButtonIndex = 0;
        }

        if (upPressed) {
            m_selectedButtonIndex = (m_selectedButtonIndex - 1 + visibleButtons.size()) % visibleButtons.size();
        }
        if (downPressed) {
            m_selectedButtonIndex = (m_selectedButtonIndex + 1) % visibleButtons.size();
        }

        // Mouse hover sync: if the mouse actively moves, let mouse hover take preference
        double mx = 0.0, my = 0.0;
        glfwGetCursorPos(engine.GetWindow().GetHandle(), &mx, &my);
        bool clicked = glfwGetMouseButton(engine.GetWindow().GetHandle(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

        bool mouseMoved = false;
        static double lastMx = 0.0, lastMy = 0.0;
        if (std::abs(mx - lastMx) > 1e-3 || std::abs(my - lastMy) > 1e-3) {
            mouseMoved = true;
            lastMx = mx;
            lastMy = my;
        }

        m_uiRenderer.UpdateMouseInput(mx, my, clicked);

        if (mouseMoved) {
            for (std::size_t i = 0; i < visibleButtons.size(); ++i) {
                if (visibleButtons[i]->hovered) {
                    m_selectedButtonIndex = static_cast<int>(i);
                }
            }
        }

        // Apply keyboard selected focus override
        for (std::size_t i = 0; i < visibleButtons.size(); ++i) {
            visibleButtons[i]->hovered = (static_cast<int>(i) == m_selectedButtonIndex);
        }

        if (confirmPressed) {
            if (visibleButtons[m_selectedButtonIndex]->onClick) {
                visibleButtons[m_selectedButtonIndex]->onClick();
            }
        }
    } else {
        // Fallback to standard mouse updates
        double mx = 0.0, my = 0.0;
        glfwGetCursorPos(engine.GetWindow().GetHandle(), &mx, &my);
        bool clicked = glfwGetMouseButton(engine.GetWindow().GetHandle(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        m_uiRenderer.UpdateMouseInput(mx, my, clicked);
    }
}

void MenuSystem::Render(Engine& engine) {
    switch (m_state) {
        case AppState::StoryIntro:
            RenderStoryIntro();
            break;
        case AppState::MainMenu:
            RenderMainMenu();
            break;
        case AppState::Settings:
            RenderSettings();
            break;
        case AppState::DifficultySelect:
            RenderDifficulty();
            break;
        case AppState::Loading:
            RenderLoading();
            break;
        case AppState::Playing:
            RenderPlaying(engine);
            break;
        case AppState::Paused:
            RenderPaused();
            break;
        case AppState::GameOver:
            RenderGameOver();
            break;
    }
}

void MenuSystem::SetState(AppState state) {
    m_state = state;
    m_uiRenderer.ClearButtons();
    m_selectedButtonIndex = 0;

    if (state == AppState::StoryIntro) {
        m_introElapsed = 0.0f;
        if (m_audioManager) {
            m_audioManager->PlaySound("intro_music", 0.65f);
        }
    }
    else if (state == AppState::MainMenu)             SetupMainMenu();
    else if (state == AppState::Settings)         SetupSettingsScreen();
    else if (state == AppState::DifficultySelect) SetupDifficultyScreen();
    else if (state == AppState::Paused)           SetupPauseMenu();
    else if (state == AppState::GameOver)         SetupGameOverScreen();
}

void MenuSystem::SetupMainMenu() {
    m_uiRenderer.LoadBackground("assets/ui/main_menu_bg.png");

    glm::vec4 normalColor = glm::vec4(0.1f, 0.1f, 0.1f, 0.75f);
    glm::vec4 hoverColor  = glm::vec4(0.2f, 0.2f, 0.2f, 0.90f);
    glm::vec4 textColor   = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

    UIButton newGameBtn;
    newGameBtn.label = "NEW GAME";
    newGameBtn.x = 0.5f;
    newGameBtn.y = 0.45f;
    newGameBtn.width  = 0.22f;
    newGameBtn.height = 0.07f;
    newGameBtn.normalColor = normalColor;
    newGameBtn.hoverColor  = hoverColor;
    newGameBtn.textColor   = textColor;
    newGameBtn.onClick = [this]() { SetState(AppState::DifficultySelect); };

    UIButton continueBtn;
    continueBtn.label = "CONTINUE";
    continueBtn.x = 0.5f;
    continueBtn.y = 0.55f;
    continueBtn.width  = 0.22f;
    continueBtn.height = 0.07f;
    continueBtn.normalColor = normalColor;
    continueBtn.hoverColor  = hoverColor;
    continueBtn.textColor   = textColor;
    continueBtn.visible = HasSaveFile();
    continueBtn.onClick = [this]() {
        if (m_engine) ContinueGame(*m_engine);
    };

    UIButton settingsBtn;
    settingsBtn.label = "SETTINGS";
    settingsBtn.x = 0.5f;
    settingsBtn.y = 0.65f;
    settingsBtn.width  = 0.22f;
    settingsBtn.height = 0.07f;
    settingsBtn.normalColor = normalColor;
    settingsBtn.hoverColor  = hoverColor;
    settingsBtn.textColor   = textColor;
    settingsBtn.onClick = [this]() { SetState(AppState::Settings); };

    m_uiRenderer.AddButton(newGameBtn);
    m_uiRenderer.AddButton(continueBtn);
    m_uiRenderer.AddButton(settingsBtn);
}

void MenuSystem::SetupDifficultyScreen() {
    m_uiRenderer.LoadBackground("assets/ui/difficulty_bg.png");

    UIButton easyBtn;
    easyBtn.label = "EASY";
    easyBtn.x = 0.28f;
    easyBtn.y = 0.55f;
    easyBtn.width  = 0.18f;
    easyBtn.height = 0.08f;
    easyBtn.normalColor = glm::vec4(0.05f, 0.2f, 0.05f, 0.8f);
    easyBtn.hoverColor  = glm::vec4(0.1f,  0.4f, 0.1f,  0.9f);
    easyBtn.textColor   = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    easyBtn.onClick = [this]() {
        m_difficulty = Difficulty::Easy;
        if (m_engine) StartNewGame(*m_engine);
    };

    UIButton normalBtn;
    normalBtn.label = "NORMAL";
    normalBtn.x = 0.50f;
    normalBtn.y = 0.55f;
    normalBtn.width  = 0.18f;
    normalBtn.height = 0.08f;
    normalBtn.normalColor = glm::vec4(0.15f, 0.15f, 0.05f, 0.8f);
    normalBtn.hoverColor  = glm::vec4(0.35f, 0.35f, 0.1f,  0.9f);
    normalBtn.textColor   = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    normalBtn.onClick = [this]() {
        m_difficulty = Difficulty::Normal;
        if (m_engine) StartNewGame(*m_engine);
    };

    UIButton hardBtn;
    hardBtn.label = "HARD";
    hardBtn.x = 0.72f;
    hardBtn.y = 0.55f;
    hardBtn.width  = 0.18f;
    hardBtn.height = 0.08f;
    hardBtn.normalColor = glm::vec4(0.25f, 0.02f, 0.02f, 0.8f);
    hardBtn.hoverColor  = glm::vec4(0.5f,  0.05f, 0.05f, 0.9f);
    hardBtn.textColor   = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    hardBtn.onClick = [this]() {
        m_difficulty = Difficulty::Hard;
        if (m_engine) StartNewGame(*m_engine);
    };

    UIButton backBtn;
    backBtn.label = "BACK";
    backBtn.x = 0.5f;
    backBtn.y = 0.72f;
    backBtn.width  = 0.14f;
    backBtn.height = 0.06f;
    backBtn.normalColor = glm::vec4(0.1f, 0.1f, 0.1f, 0.75f);
    backBtn.hoverColor  = glm::vec4(0.2f, 0.2f, 0.2f, 0.9f);
    backBtn.textColor   = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
    backBtn.onClick = [this]() { SetState(AppState::MainMenu); };

    m_uiRenderer.AddButton(easyBtn);
    m_uiRenderer.AddButton(normalBtn);
    m_uiRenderer.AddButton(hardBtn);
    m_uiRenderer.AddButton(backBtn);
}

void MenuSystem::SetupSettingsScreen() {
    m_uiRenderer.LoadBackground("assets/ui/settings_bg.png");

    UIButton backBtn;
    backBtn.label = "BACK";
    backBtn.x = 0.5f;
    backBtn.y = 0.82f;
    backBtn.width  = 0.16f;
    backBtn.height = 0.06f;
    backBtn.normalColor = glm::vec4(0.1f, 0.1f, 0.1f, 0.8f);
    backBtn.hoverColor  = glm::vec4(0.25f, 0.25f, 0.25f, 0.9f);
    backBtn.textColor   = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    backBtn.onClick = [this]() { SetState(AppState::MainMenu); };

    m_uiRenderer.AddButton(backBtn);
}

void MenuSystem::SetupPauseMenu() {
    m_uiRenderer.LoadBackground("assets/ui/pause_bg.png");

    glm::vec4 normalColor = glm::vec4(0.1f, 0.1f, 0.1f, 0.8f);
    glm::vec4 hoverColor  = glm::vec4(0.2f, 0.2f, 0.2f, 0.9f);
    glm::vec4 textColor   = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

    UIButton resumeBtn;
    resumeBtn.label = "CONTINUE";
    resumeBtn.x = 0.5f;
    resumeBtn.y = 0.45f;
    resumeBtn.width  = 0.22f;
    resumeBtn.height = 0.07f;
    resumeBtn.normalColor = normalColor;
    resumeBtn.hoverColor  = hoverColor;
    resumeBtn.textColor   = textColor;
    resumeBtn.onClick = [this]() {
        SetState(AppState::Playing);
        if (m_game) {
            m_game->SetCursorLocked(true);
        }
        if (m_engine) {
            m_engine->GetInput().SetCursorLocked(true);
        }
    };

    UIButton saveBtn;
    saveBtn.label = "SAVE GAME";
    saveBtn.x = 0.5f;
    saveBtn.y = 0.55f;
    saveBtn.width  = 0.22f;
    saveBtn.height = 0.07f;
    saveBtn.normalColor = normalColor;
    saveBtn.hoverColor  = hoverColor;
    saveBtn.textColor   = textColor;
    saveBtn.visible = !m_hardModeNoSave;
    saveBtn.onClick = [this]() { SaveGame(); };

    UIButton quitBtn;
    quitBtn.label = "QUIT TO MENU";
    quitBtn.x = 0.5f;
    quitBtn.y = 0.65f;
    quitBtn.width  = 0.22f;
    quitBtn.height = 0.07f;
    quitBtn.normalColor = normalColor;
    quitBtn.hoverColor  = hoverColor;
    quitBtn.textColor   = textColor;
    quitBtn.onClick = [this]() { QuitGame(); };

    m_uiRenderer.AddButton(resumeBtn);
    m_uiRenderer.AddButton(saveBtn);
    m_uiRenderer.AddButton(quitBtn);
}

void MenuSystem::SetupGameOverScreen() {
    m_uiRenderer.LoadBackground("assets/ui/gameover_bg.png");

    UIButton tryAgainBtn;
    tryAgainBtn.label = "TRY AGAIN";
    tryAgainBtn.x = 0.5f;
    tryAgainBtn.y = 0.58f;
    tryAgainBtn.width  = 0.20f;
    tryAgainBtn.height = 0.07f;
    tryAgainBtn.normalColor = glm::vec4(0.25f, 0.02f, 0.02f, 0.8f);
    tryAgainBtn.hoverColor  = glm::vec4(0.5f,  0.05f, 0.05f, 0.9f);
    tryAgainBtn.textColor   = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    tryAgainBtn.onClick = [this]() {
        if (m_engine) StartNewGame(*m_engine);
    };

    UIButton menuBtn;
    menuBtn.label = "MAIN MENU";
    menuBtn.x = 0.5f;
    menuBtn.y = 0.68f;
    menuBtn.width  = 0.20f;
    menuBtn.height = 0.07f;
    menuBtn.normalColor = glm::vec4(0.1f, 0.1f, 0.1f, 0.8f);
    menuBtn.hoverColor  = glm::vec4(0.2f, 0.2f, 0.2f, 0.9f);
    menuBtn.textColor   = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
    menuBtn.onClick = [this]() { QuitGame(); };

    m_uiRenderer.AddButton(tryAgainBtn);
    m_uiRenderer.AddButton(menuBtn);
}

void MenuSystem::UpdateLoading(float dt, Engine& engine) {
    m_loadingDisplayTimer += dt;
    if (m_loadingDisplayTimer >= 0.25f) {
        m_loadingDisplayTimer = 0.0f;

        if (m_loadPhase == 0) {
            m_game->InitializePhase0(engine);
            m_game->ApplyDifficulty(m_difficulty);
            m_loadPhase = 1;
        } else if (m_loadPhase == 1) {
            m_game->InitializePhase1(engine);
            m_loadPhase = 2;
        } else if (m_loadPhase == 2) {
            m_game->InitializePhase2(engine);
            m_loadPhase = 3;
        } else if (m_loadPhase == 3) {
            bool moreToLoad = false;
            if (m_game && m_game->GetWorld()) {
                moreToLoad = m_game->GetWorld()->LoadNextPendingMesh();
            }
            if (!moreToLoad) {
                m_loadPhase = 4;
            }
        } else if (m_loadPhase == 4) {
            bool moreToLoad = m_game->LoadNextCharacterMesh();
            if (!moreToLoad) {
                m_loadPhase = 5;
            }
        } else if (m_loadPhase == 5) {
            // Apply volume
            if (m_game->GetWorld() && m_game->GetWorld()->GetAudioManager()) {
                m_game->GetWorld()->GetAudioManager()->SetMasterVolume(m_masterVolume);
            }
            // Apply difficulty
            m_game->ApplyDifficulty(m_difficulty);

            // Restore save if applicable
            if (m_shouldLoadSave) {
                m_game->LoadGame();
            }

            m_gameInitialized = true;
            m_pauseInputCooldown = 0.3f;
            SetState(AppState::Playing);
            m_game->SetCursorLocked(true);
            engine.GetInput().SetCursorLocked(true);
        }
    }
}

void MenuSystem::UpdatePlaying(float dt, Engine& engine) {
    if (m_pauseInputCooldown > 0.0f) {
        m_pauseInputCooldown -= dt;
    }

    if (m_pauseInputCooldown <= 0.0f && engine.GetGameplayInput().IsActionPressed(InputAction::Pause)) {
        SetState(AppState::Paused);
        m_pauseInputCooldown = 0.3f;
        if (m_game) {
            m_game->SetCursorLocked(false);
        }
        engine.GetInput().SetCursorLocked(false);
        return;
    }

    if (m_game) {
        m_game->Update(dt, engine);

        if (m_game->HasAnyCharacterDied()) {
            m_deadCharacterName = m_game->GetLastDeadCharacterName();
            SetState(AppState::GameOver);
            m_game->SetCursorLocked(false);
            engine.GetInput().SetCursorLocked(false);
        }
    }
}

void MenuSystem::RenderMainMenu() {
    m_uiRenderer.RenderBackground();
    m_uiRenderer.RenderTitle("FROMVILLE", 0.5f, 0.22f, 1.3f, glm::vec4(1.0f, 0.72f, 0.18f, 1.0f));
    m_uiRenderer.RenderText("NO WAY OUT", 0.5f, 0.30f, 0.65f, glm::vec4(0.85f, 0.85f, 0.90f, 1.0f));
    m_uiRenderer.RenderButtons();
    m_uiRenderer.RenderText("DESIGN BY GOOGLE DEEPMIND TEAM", 0.5f, 0.92f, 0.38f, glm::vec4(0.45f, 0.48f, 0.52f, 1.0f));
}

void MenuSystem::RenderDifficulty() {
    m_uiRenderer.RenderBackground();
    m_uiRenderer.RenderTitle("CHOOSE DIFFICULTY", 0.5f, 0.22f, 1.1f, glm::vec4(1.0f, 0.72f, 0.18f, 1.0f));
    m_uiRenderer.RenderText("EASY: 150 MAX HP, 7.5 DAMAGE", 0.28f, 0.38f, 0.40f, glm::vec4(0.6f, 0.9f, 0.6f, 1.0f));
    m_uiRenderer.RenderText("NORMAL: 100 MAX HP, 15 DAMAGE", 0.50f, 0.38f, 0.40f, glm::vec4(0.9f, 0.9f, 0.6f, 1.0f));
    m_uiRenderer.RenderText("HARD: 100 MAX HP, 20 DAMAGE [NO SAVE]", 0.72f, 0.38f, 0.40f, glm::vec4(0.9f, 0.5f, 0.5f, 1.0f));
    m_uiRenderer.RenderButtons();
}

void MenuSystem::RenderSettings() {
    m_uiRenderer.RenderBackground();
    m_uiRenderer.RenderTitle("SETTINGS", 0.5f, 0.22f, 1.2f, glm::vec4(1.0f, 0.72f, 0.18f, 1.0f));
    m_uiRenderer.RenderText("MASTER VOLUME", 0.5f, 0.45f, 0.60f, glm::vec4(0.85f, 0.85f, 0.90f, 1.0f));
    
    std::string volStr = "[ - ]         " + std::to_string(static_cast<int>(m_masterVolume * 100.0f)) + "%" + "         [ + ]";
    m_uiRenderer.RenderText(volStr, 0.5f, 0.62f, 0.55f, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    m_uiRenderer.RenderButtons();
}

void MenuSystem::RenderLoading() {
    m_uiRenderer.RenderBackgroundOverlay(0.85f);
    m_uiRenderer.RenderTitle("LOADING RESOURCE DATABASE...", 0.5f, 0.40f, 0.85f, glm::vec4(1.0f, 0.72f, 0.18f, 1.0f));

    m_uiRenderer.RenderProgressBar(0.5f, 0.55f, 0.5f, 0.025f, static_cast<float>(m_loadPhase) / 5.0f, glm::vec4(0.12f, 0.12f, 0.15f, 0.85f), glm::vec4(1.0f, 0.72f, 0.18f, 1.0f));

    std::string desc;
    switch (m_loadPhase) {
        case 0: desc = "PHASE 0: CREATING VIRTUAL CollisionWorld SYSTEM..."; break;
        case 1: desc = "PHASE 1: INSTANTIATING GROUND AND TERRAIN RENDERER..."; break;
        case 2: desc = "PHASE 2: COMPILED SHADER ENGINE GRAPHICS..."; break;
        case 3: desc = "PHASE 3: IMPORTING CHARACTER GEOMETRY MESH DATA..."; break;
        case 4: desc = "PHASE 4: SYNCING LOCOMOTION STANDING IDLE ANIMATIONS..."; break;
        case 5: desc = "PHASE 5: DATABASE SYNCHRONIZATION COMPLETE."; break;
    }
    m_uiRenderer.RenderText(desc, 0.5f, 0.65f, 0.45f, glm::vec4(0.85f, 0.88f, 0.92f, 1.0f));
}

void MenuSystem::RenderPlaying(Engine& engine) {
    if (m_game) {
        m_game->Render(engine);
    }
}

void MenuSystem::RenderPaused() {
    m_uiRenderer.RenderBackgroundOverlay(0.68f);
    m_uiRenderer.RenderTitle("GAME SUSPENDED", 0.5f, 0.28f, 1.1f, glm::vec4(1.0f, 0.72f, 0.18f, 1.0f));
    m_uiRenderer.RenderButtons();
}

void MenuSystem::RenderGameOver() {
    m_uiRenderer.RenderBackgroundOverlay(0.85f);
    m_uiRenderer.RenderTitle("UNIT TERMINATED", 0.5f, 0.32f, 1.2f, glm::vec4(1.0f, 0.2f, 0.2f, 1.0f));
    
    std::string deadMsg = "LOST CONTACT WITH UNIT " + m_deadCharacterName;
    m_uiRenderer.RenderText(deadMsg, 0.5f, 0.45f, 0.55f, glm::vec4(0.85f, 0.85f, 0.90f, 1.0f));
    m_uiRenderer.RenderButtons();
}

bool MenuSystem::HasSaveFile() const {
    return std::filesystem::exists("savegame.txt");
}

void MenuSystem::StartNewGame(Engine& engine) {
    if (m_difficulty == Difficulty::Hard) {
        m_hardModeNoSave = true;
    } else {
        m_hardModeNoSave = false;
    }

    if (m_game) {
        delete m_game;
        m_game = nullptr;
    }
    m_game = new Game();
    m_game->SetLoadStateToReady();
    m_loadPhase = 0;
    m_loadingDisplayTimer = 0.0f;
    m_shouldLoadSave = false;

    SetState(AppState::Loading);
}

void MenuSystem::ContinueGame(Engine& engine) {
    if (m_game) {
        delete m_game;
        m_game = nullptr;
    }
    m_game = new Game();
    m_game->SetLoadStateToReady();
    m_loadPhase = 0;
    m_loadingDisplayTimer = 0.0f;
    m_shouldLoadSave = true;

    SetState(AppState::Loading);
}

void MenuSystem::SaveGame() {
    if (m_hardModeNoSave) {
        std::cout << "[MenuSystem] Saving is disabled in Hard Mode!\n";
        return;
    }
    if (m_game) {
        m_game->SaveGame();
    }
}

void MenuSystem::QuitGame() {
    if (m_game) {
        delete m_game;
        m_game = nullptr;
    }
    m_gameInitialized = false;
    SetState(AppState::MainMenu);
}

void MenuSystem::UpdateStoryIntro(float dt, Engine& engine) {
    m_introElapsed += dt;
    if (engine.GetInput().IsKeyPressed(GLFW_KEY_ENTER) || engine.GetInput().IsKeyPressed(GLFW_KEY_SPACE)) {
        if (m_audioManager) {
            m_audioManager->StopSound("intro_music");
        }
        SetState(AppState::MainMenu);
    }
}

void MenuSystem::RenderStoryIntro() {
    // Clear screen to premium deep charcoal/black background
    glClearColor(0.02f, 0.02f, 0.03f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    auto getFadeAlpha = [this](float startTime) -> float {
        if (m_introElapsed < startTime) return 0.0f;
        return std::min(1.0f, (m_introElapsed - startTime) / 1.5f); // 1.5s fade-in duration
    };

    // Pulse factor for text elements
    float pulse = 0.7f + 0.3f * std::sin(m_introElapsed * 4.0f);

    // --- ULTRA-SMOOTH CONTINUOUS INTERPOLATION ---
    float t_smooth = 0.0f;
    if (m_introElapsed >= 5.5f) {
        float t_raw = std::min(1.0f, (m_introElapsed - 5.5f) / 1.5f);
        t_smooth = t_raw * t_raw * (3.0f - 2.0f * t_raw); // smoothstep s-curve
    }

    // Settle float factor (floating motion dampens down as paper unfolds)
    float floatScale = 1.0f - t_smooth;
    float floatX = std::cos(m_introElapsed * 2.2f) * 20.0f * floatScale;
    float floatY = std::sin(m_introElapsed * 2.8f) * 25.0f * floatScale;

    float startW = 600.0f;
    float startH = 160.0f;
    float targetW = static_cast<float>(m_screenWidth) - 100.0f;
    float targetH = static_cast<float>(m_screenHeight) - 80.0f;

    float curW = startW + t_smooth * (targetW - startW);
    float curH = startH + t_smooth * (targetH - startH);

    float curX = (static_cast<float>(m_screenWidth) / 2.0f) - (curW / 2.0f) + floatX;
    float curY = (static_cast<float>(m_screenHeight) / 2.0f) - (curH / 2.0f) + floatY;

    // Fades for overall background / card (starts at 0.0, reaches 1.0 at 1.5 seconds)
    float paperAlpha = std::min(1.0f, m_introElapsed / 1.5f);

    glm::vec3 darkShadow = glm::vec3(0.12f, 0.09f, 0.07f) * paperAlpha; // card shadow
    glm::vec3 paperColor = glm::vec3(0.88f, 0.83f, 0.73f) * paperAlpha; // warm parchment beige

    // Procedural torn paper edges
    auto drawTornPaper = [&](float px, float py, float pw, float ph, const glm::vec3& pcolor) {
        const int N = 32;
        float sliceHeight = ph / static_cast<float>(N);
        for (int i = 0; i < N; ++i) {
            float sliceY = py + static_cast<float>(i) * sliceHeight;
            float edgeOffsetL = std::sin(static_cast<float>(i) * 0.9f) * 4.0f + std::cos(static_cast<float>(i) * 2.3f) * 2.0f;
            float edgeOffsetR = std::cos(static_cast<float>(i) * 1.1f) * 4.0f + std::sin(static_cast<float>(i) * 2.7f) * 2.0f;
            
            float tearIn = 0.0f;
            if (i == 4)  tearIn = 16.0f;
            if (i == 14) tearIn = -14.0f;
            if (i == 24) tearIn = 18.0f;
            
            float finalSliceX = px + edgeOffsetL + (tearIn > 0.0f ? tearIn : 0.0f);
            float finalSliceW = pw + edgeOffsetR - edgeOffsetL - std::abs(tearIn);
            
            float finalSliceH = sliceHeight + 1.5f;
            if (i == 0 || i == N - 1) {
                finalSliceH -= 2.0f;
            }
            if (m_textRenderer) {
                m_textRenderer->RenderQuad(finalSliceX, sliceY, finalSliceW, finalSliceH, pcolor, m_screenWidth, m_screenHeight);
            }
        }
    };

    // 1. Render Floating Paper Plane drop shadow
    drawTornPaper(curX - 4.0f, curY - 4.0f, curW + 8.0f, curH + 8.0f, darkShadow);
    // 2. Render Floating Paper Plane primary parchment sheet
    drawTornPaper(curX, curY, curW, curH, paperColor);

    // --- STAGE 2 TEXT (context story, objectives, character backgrounds) ---
    if (t_smooth > 0.0f) {
        std::string separator = "~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~";
        float sepScale = 0.45f;
        float sepX = curX + (curW / 2.0f) - (separator.length() * 10.0f * sepScale / 2.0f);

        // Title (Dark crimson blood ink)
        std::string title = "♦  FROMVILLE: THE NIGHTMARE BEGINS  ♦";
        float titleScale = 1.3f;
        float titleX = curX + (curW / 2.0f) - (title.length() * 14.0f * titleScale / 2.0f);
        m_textRenderer->RenderText(title, titleX, curY + curH - 50.0f, titleScale, glm::vec3(0.75f, 0.10f, 0.10f), m_screenWidth, m_screenHeight, getFadeAlpha(7.0f));

        // --- 1. THE STORY BACKGROUND ---
        std::string storyHeader = "THE MYSTERY & BACKGROUND";
        m_textRenderer->RenderText(storyHeader, curX + 50.0f, curY + curH - 110.0f, 0.55f, glm::vec3(0.40f, 0.20f, 0.05f), m_screenWidth, m_screenHeight, getFadeAlpha(8.0f));

        std::string storyLine1 = "You are trapped in a mysterious town in middle America that traps everyone who enters.";
        std::string storyLine2 = "The roads loop infinitely back to town. The forest is thick, dark, and alive with ancient power.";
        std::string storyLine3 = "And at night, nightmarish monsters crawl out of the woods. They don't run, they don't hide...";
        std::string storyLine4 = "They walk calmly. They smile. And if you let them in, they will tear you apart.";
        
        m_textRenderer->RenderText(storyLine1, curX + 50.0f, curY + curH - 145.0f, 0.42f, glm::vec3(0.18f, 0.15f, 0.12f), m_screenWidth, m_screenHeight, getFadeAlpha(9.0f));
        m_textRenderer->RenderText(storyLine2, curX + 50.0f, curY + curH - 170.0f, 0.42f, glm::vec3(0.18f, 0.15f, 0.12f), m_screenWidth, m_screenHeight, getFadeAlpha(10.0f));
        m_textRenderer->RenderText(storyLine3, curX + 50.0f, curY + curH - 195.0f, 0.42f, glm::vec3(0.18f, 0.15f, 0.12f), m_screenWidth, m_screenHeight, getFadeAlpha(11.0f));
        m_textRenderer->RenderText(storyLine4, curX + 50.0f, curY + curH - 220.0f, 0.42f, glm::vec3(0.18f, 0.15f, 0.12f), m_screenWidth, m_screenHeight, getFadeAlpha(12.0f));

        // First Separator
        m_textRenderer->RenderText(separator, sepX, curY + curH - 250.0f, sepScale, glm::vec3(0.60f, 0.52f, 0.42f), m_screenWidth, m_screenHeight, getFadeAlpha(13.0f));

        // --- 2. YOUR MISSIONS ---
        std::string missionsHeader = "YOUR SURVIVAL OBJECTIVES";
        m_textRenderer->RenderText(missionsHeader, curX + 50.0f, curY + curH - 280.0f, 0.55f, glm::vec3(0.40f, 0.20f, 0.05f), m_screenWidth, m_screenHeight, getFadeAlpha(13.5f));

        std::string mission1 = "• SURVIVE THE NOCTURNAL HUNT: Secure safety inside houses before the sun sets.";
        std::string mission2 = "• PUZZLE SOLVING: Only Tabitha and Jade can decipher ancient talisman puzzles at night.";
        std::string mission3 = "• MAINTAIN COMMUNITY: Watch over the townspeople who roam doing chores during the day.";

        m_textRenderer->RenderText(mission1, curX + 50.0f, curY + curH - 315.0f, 0.42f, glm::vec3(0.10f, 0.22f, 0.38f), m_screenWidth, m_screenHeight, getFadeAlpha(14.5f));
        m_textRenderer->RenderText(mission2, curX + 50.0f, curY + curH - 340.0f, 0.42f, glm::vec3(0.10f, 0.22f, 0.38f), m_screenWidth, m_screenHeight, getFadeAlpha(15.5f));
        m_textRenderer->RenderText(mission3, curX + 50.0f, curY + curH - 365.0f, 0.42f, glm::vec3(0.10f, 0.22f, 0.38f), m_screenWidth, m_screenHeight, getFadeAlpha(16.5f));

        // Second Separator
        m_textRenderer->RenderText(separator, sepX, curY + curH - 395.0f, sepScale, glm::vec3(0.60f, 0.52f, 0.42f), m_screenWidth, m_screenHeight, getFadeAlpha(18.0f));

        // --- 3. CHARACTER BACKGROUNDS ---
        std::string charHeader = "PLAYABLE SURVIVOR ROLES";
        m_textRenderer->RenderText(charHeader, curX + 50.0f, curY + curH - 430.0f, 0.55f, glm::vec3(0.40f, 0.20f, 0.05f), m_screenWidth, m_screenHeight, getFadeAlpha(18.5f));

        std::string charBoyd    = "• SHERIFF BOYD: The weary leader keeping order. Spawns with 100% resolve.";
        std::string charJade    = "• JADE HERERA: Brilliant, arrogant tech-mogul. Deciphers mathematical stones.";
        std::string charTabitha = "• TABITHA MATTHEWS: Determined mother looking for her child and the lighthouse exit.";
        std::string charVictor  = "• VICTOR: Mysterious artist who survived here for decades. Knows hidden paths.";

        m_textRenderer->RenderText(charBoyd, curX + 50.0f, curY + curH - 465.0f, 0.42f, glm::vec3(0.20f, 0.24f, 0.28f), m_screenWidth, m_screenHeight, getFadeAlpha(19.5f));
        m_textRenderer->RenderText(charJade, curX + 50.0f, curY + curH - 490.0f, 0.42f, glm::vec3(0.20f, 0.24f, 0.28f), m_screenWidth, m_screenHeight, getFadeAlpha(20.5f));
        m_textRenderer->RenderText(charTabitha, curX + 50.0f, curY + curH - 515.0f, 0.42f, glm::vec3(0.20f, 0.24f, 0.28f), m_screenWidth, m_screenHeight, getFadeAlpha(21.5f));
        m_textRenderer->RenderText(charVictor, curX + 50.0f, curY + curH - 540.0f, 0.42f, glm::vec3(0.20f, 0.24f, 0.28f), m_screenWidth, m_screenHeight, getFadeAlpha(22.5f));
    }

    // --- STAGE 1 TEXT (typewriter warning) ---
    if (m_introElapsed >= 1.5f && m_introElapsed < 5.5f) {
        float warningAlpha = 1.0f;
        if (m_introElapsed >= 4.5f) {
            warningAlpha = std::max(0.0f, 1.0f - (m_introElapsed - 4.5f) / 1.0f);
        }

        glm::vec3 phraseColor = glm::vec3(0.75f, 0.10f, 0.10f);
        glm::vec3 stampColor = glm::vec3(0.40f, 0.30f, 0.15f);

        std::string phrase = "KNOWLEDGE COMES AT A COST...";
        float typingSpeed = 14.0f;
        float typeElapsed = m_introElapsed - 1.5f;
        int charsToShow = std::min(static_cast<int>(phrase.length()), static_cast<int>(typeElapsed * typingSpeed));
        std::string typedPhrase = phrase.substr(0, charsToShow);
        
        if (charsToShow < static_cast<int>(phrase.length()) && (static_cast<int>(typeElapsed * 4.0f) % 2 == 0)) {
            typedPhrase += "_";
        }

        float pScale = 0.55f;
        float pX = curX + (curW / 2.0f) - (phrase.length() * 10.0f * pScale / 2.0f);
        float pY = curY + (curH / 2.0f) + 15.0f;

        m_textRenderer->RenderText(typedPhrase, pX, pY, pScale, phraseColor, m_screenWidth, m_screenHeight, warningAlpha);

        std::string subText = "--- Journal Entry #1978 ---";
        float sScale = 0.40f;
        float sX = curX + (curW / 2.0f) - (subText.length() * 10.0f * sScale / 2.0f);
        float sY = curY + (curH / 2.0f) - 25.0f;
        
        m_textRenderer->RenderText(subText, sX, sY, sScale, stampColor, m_screenWidth, m_screenHeight, warningAlpha);
    }

    // --- 4. PROMPT TO CONTINUE ---
    std::string prompt = "PRESS [SPACE] OR [ENTER] TO CONTINUE";
    float promptScale = 0.55f;
    float promptX = (static_cast<float>(m_screenWidth) / 2.0f) - (prompt.length() * 10.0f * promptScale / 2.0f);
    
    glm::vec3 promptBaseColor = (m_introElapsed < 4.0f) ? glm::vec3(1.0f, 0.72f, 0.18f) : glm::vec3(0.45f, 0.22f, 0.05f);
    float promptAlpha = getFadeAlpha(1.8f);
    m_textRenderer->RenderText(prompt, promptX, 50.0f, promptScale, promptBaseColor, m_screenWidth, m_screenHeight, promptAlpha * pulse);
}
