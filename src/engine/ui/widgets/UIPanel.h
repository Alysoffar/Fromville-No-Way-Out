#pragma once

#include "engine/ui/UIWidget.h"
#include <vector>
#include <memory>

class UIWidget;

class UIPanel : public UIWidget {
public:
    void AddChild(std::unique_ptr<UIWidget> child);
    void Update(float dt, InputContext& input) override;
    void Render(TextRenderer* hud, int screenWidth, int screenHeight) override;

private:
    std::vector<std::unique_ptr<UIWidget>> children;
};
