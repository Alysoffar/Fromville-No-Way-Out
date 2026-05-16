#pragma once

#include <glm/vec2.hpp>
#include <string>

class TextRenderer;
class InputContext;

class UIWidget {
public:
    UIWidget() = default;
    virtual ~UIWidget() = default;

    virtual void Update(float dt, InputContext& input) {}
    virtual void Render(TextRenderer* hud, int screenWidth, int screenHeight) {}

    void SetPosition(const glm::vec2& p) { pos = p; }
    const glm::vec2& GetPosition() const { return pos; }

    void SetVisible(bool v) { visible = v; }
    bool IsVisible() const { return visible; }

    void SetFocus(bool f) { focused = f; }
    bool HasFocus() const { return focused; }

protected:
    glm::vec2 pos{0.0f, 0.0f};
    bool visible = true;
    bool focused = false;
};
