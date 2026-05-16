#include "engine/ui/widgets/UIButton.h"
#include "engine/renderer/TextRenderer.h"
#include "engine/input/InputContext.h"
#include "engine/input/InputAction.h"

#include <glm/vec3.hpp>
#include <algorithm>

UIButton::UIButton(std::string label) : label(std::move(label)) {}

void UIButton::Update(float dt, InputContext& input) {
    hoverTime += dt;
    // selection handled by UIScreen navigation; confirm triggers callback
    if (selected && enabled && input.IsActionPressed(InputAction::Confirm)) {
        if (callback) callback();
    }
}

void UIButton::Render(TextRenderer* hud, int screenWidth, int screenHeight) {
    if (!hud || !visible) return;

    glm::vec3 color(0.78f, 0.78f, 0.78f);
    if (!enabled) color = glm::vec3(0.4f);
    if (selected) color = glm::vec3(0.96f, 0.92f, 0.78f);

    hud->RenderText(label, pos.x, pos.y, 1.0f, color, screenWidth, screenHeight, false);
}
