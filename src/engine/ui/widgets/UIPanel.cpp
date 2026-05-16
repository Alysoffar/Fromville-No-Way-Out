#include "engine/ui/widgets/UIPanel.h"
#include "engine/input/InputContext.h"
#include "engine/renderer/TextRenderer.h"

void UIPanel::AddChild(std::unique_ptr<UIWidget> child) {
    children.push_back(std::move(child));
}

void UIPanel::Update(float dt, InputContext& input) {
    if (!visible) return;
    for (auto& c : children) {
        c->Update(dt, input);
    }
}

void UIPanel::Render(TextRenderer* hud, int screenWidth, int screenHeight) {
    if (!visible) return;
    for (auto& c : children) {
        c->Render(hud, screenWidth, screenHeight);
    }
}
