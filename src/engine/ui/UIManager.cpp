#include "engine/ui/UIManager.h"
#include "engine/ui/UIScreen.h"
#include "engine/ui/TransitionManager.h"

#include "engine/renderer/TextRenderer.h"
#include "engine/input/InputContext.h"

UIManager& UIManager::Instance() {
    static UIManager inst;
    return inst;
}

UIManager::UIManager() = default;

void UIManager::Initialize(TextRenderer* hudRenderer) {
    hud = hudRenderer;
}

void UIManager::Shutdown() {
    screens.clear();
}

void UIManager::Update(float dt, InputContext& input) {
    TransitionManager::Instance().Update(dt);

    if (!screens.empty()) {
        auto* top = screens.back().get();
        if (top) top->Update(dt, input);
    }
}

void UIManager::Render(int screenWidth, int screenHeight) {
    if (!screens.empty()) {
        auto* top = screens.back().get();
        if (top) top->Render(hud, screenWidth, screenHeight);
    }

    // fade handling reserved for later renderer integration
    (void)screenWidth; (void)screenHeight;
}

void UIManager::PushScreen(std::unique_ptr<UIScreen> screen) {
    if (!screen) return;
    screen->OnEnter();
    screens.push_back(std::move(screen));
}

void UIManager::PopScreen() {
    if (screens.empty()) return;
    screens.back()->OnExit();
    screens.pop_back();
}

UIScreen* UIManager::GetTopScreen() {
    if (screens.empty()) return nullptr;
    return screens.back().get();
}
