#pragma once

#include <string>
#include <memory>
#include <vector>

class TextRenderer;
class InputContext;

class UIScreen {
public:
    virtual ~UIScreen() = default;
    virtual void OnEnter() {}
    virtual void OnExit() {}
    virtual void Update(float dt, InputContext& input) = 0;
    virtual void Render(TextRenderer* hud, int screenWidth, int screenHeight) = 0;

    virtual bool IsModal() const { return false; }
    // helper to route confirm/cancel to focused widget
    virtual void HandleConfirm(InputContext& input) {}
    virtual void HandleCancel(InputContext& input) {}
};
