#pragma once
#include "game/world/World.h"
#include "engine/renderer/UIRenderer.h"
#include "engine/audio/AudioManager.h"
#include <string>
#include <memory>

enum class AppState {
    StoryIntro,
    MainMenu,
    Settings,
    DifficultySelect,
    Loading,
    Playing,
    Paused,
    GameOver
};

#include "engine/renderer/TextRenderer.h"
class Engine;
class Game;

class MenuSystem {
public:
    bool Initialize(Engine& engine, int screenWidth, int screenHeight);
    void Shutdown();

    void Update(float dt, Engine& engine);
    void Render(Engine& engine);

    AppState GetState() const { return m_state; }
    void SetState(AppState state);

    Difficulty GetDifficulty() const { return m_difficulty; }
    bool IsGameInitialized() const { return m_gameInitialized; }

    // Called by main loop to get the game pointer after loading
    Game* GetGame() { return m_game; }

private:
    AppState m_state = AppState::StoryIntro;
    Difficulty m_difficulty = Difficulty::Normal;

    UIRenderer m_uiRenderer;
    std::unique_ptr<TextRenderer> m_textRenderer;
    std::unique_ptr<AudioManager> m_audioManager;

    // Screens
    void SetupMainMenu();
    void SetupSettingsScreen();
    void SetupDifficultyScreen();
    void SetupPauseMenu();
    void SetupGameOverScreen();

    // Update per screen
    void UpdateStoryIntro(float dt, Engine& engine);
    void UpdateLoading(float dt, Engine& engine);
    void UpdatePlaying(float dt, Engine& engine);

    // Render per screen
    void RenderStoryIntro();
    void RenderMainMenu();
    void RenderSettings();
    void RenderDifficulty();
    void RenderLoading();
    void RenderPlaying(Engine& engine);
    void RenderPaused();
    void RenderGameOver();

    // Save/load
    bool HasSaveFile() const;
    void StartNewGame(Engine& engine);
    void ContinueGame(Engine& engine);
    void SaveGame();
    void QuitGame();

    // Loading state
    int m_loadPhase = 0;
    float m_loadingDisplayTimer = 0.0f;
    float m_introElapsed = 0.0f;

    // Game pointer — null until loading completes
    Game* m_game = nullptr;
    bool m_gameInitialized = false;
    bool m_startupLoading = true;
    int m_selectedButtonIndex = 0;

    // Engine back-reference
    Engine* m_engine = nullptr;

    // Settings state
    float m_masterVolume = 1.0f;
    bool m_settingsChanged = false;

    // Game over state
    std::string m_deadCharacterName;

    // Pause input cooldown to prevent immediate unpause
    float m_pauseInputCooldown = 0.0f;

    int m_screenWidth = 1280;
    int m_screenHeight = 720;

    // Save/load and difficulty modifiers flags
    bool m_shouldLoadSave = false;
    bool m_hardModeNoSave = false;
};
