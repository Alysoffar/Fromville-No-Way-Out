#pragma once

#include <memory>
#include <vector>
#include <string>

class UIScreen;
class TextRenderer;
class InputContext;

class UIManager {
public:
    static UIManager& Instance();

    void Initialize(TextRenderer* hudRenderer);
    void Shutdown();

    void Update(float dt, InputContext& input);
    void Render(int screenWidth, int screenHeight);

    void PushScreen(std::unique_ptr<UIScreen> screen);
    void PopScreen();
    UIScreen* GetTopScreen();

private:
    UIManager();
    std::vector<std::unique_ptr<UIScreen>> screens;
    TextRenderer* hud = nullptr;
};
