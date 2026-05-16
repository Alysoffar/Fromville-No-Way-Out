#pragma once

#include "engine/ui/UIScreen.h"
#include <functional>
#include <vector>
#include <string>

class MainMenuScreen : public UIScreen {
public:
    MainMenuScreen();
    void OnEnter() override;
    void OnExit() override;
    void Update(float dt, InputContext& input) override;
    void Render(TextRenderer* hud, int screenWidth, int screenHeight) override;
    bool IsModal() const override;
    void SetNewGameCallback(std::function<void()> cb) { onNewGame = std::move(cb); }
    void SetContinueCallback(std::function<void()> cb) { onContinue = std::move(cb); }
    void SetLoadCallback(std::function<void()> cb) { onLoad = std::move(cb); }
    void SetQuitCallback(std::function<void()> cb) { onQuit = std::move(cb); }

private:
    std::vector<std::string> items;
    int selected = 0;
    float blinkTimer = 0.0f;
    std::function<void()> onNewGame;
    std::function<void()> onContinue;
    std::function<void()> onLoad;
    std::function<void()> onQuit;
};
