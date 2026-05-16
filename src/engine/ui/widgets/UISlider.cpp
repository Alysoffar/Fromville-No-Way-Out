#include "engine/ui/widgets/UISlider.h"
#include "engine/renderer/TextRenderer.h"
#include "engine/input/InputContext.h"
#include "engine/input/InputAction.h"

#include <glm/vec3.hpp>

UISlider::UISlider(float min, float max)
    : minValue(min), maxValue(max), value(min) {}

void UISlider::Update(float dt, InputContext& input) {
    if (!visible) return;
    // simple keyboard adjustments when focused
    if (IsVisible() && HasFocus()) {
        if (input.IsActionPressed(InputAction::PuzzleDecrease)) {
            value = std::max(minValue, value - 0.05f);
        }
        if (input.IsActionPressed(InputAction::PuzzleIncrease)) {
            value = std::min(maxValue, value + 0.05f);
        }
    }
}

void UISlider::Render(TextRenderer* hud, int screenWidth, int screenHeight) {
    if (!hud || !visible) return;
    // render label-ish slider representation
    char buf[64];
    sprintf(buf, "[%.2f]", value);
    hud->RenderText(buf, pos.x, pos.y, 0.9f, glm::vec3(0.8f), screenWidth, screenHeight, false);
}
