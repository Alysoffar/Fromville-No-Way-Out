#pragma once

#include "engine/ui/UIWidget.h"
#include <functional>
#include <string>

class TextRenderer;
class InputContext;

class UIButton : public UIWidget {
public:
    using Callback = std::function<void()>;

    UIButton(std::string label);
    void SetCallback(Callback cb) { callback = std::move(cb); }

    void Update(float dt, InputContext& input) override;
    void Render(TextRenderer* hud, int screenWidth, int screenHeight) override;

    void SetEnabled(bool e) { enabled = e; }
    bool IsEnabled() const { return enabled; }

    void SetSelected(bool s) { selected = s; }
    bool IsSelected() const { return selected; }

private:
    std::string label;
    Callback callback;
    bool hovered = false;
    bool selected = false;
    bool enabled = true;
    float hoverTime = 0.0f;
};
