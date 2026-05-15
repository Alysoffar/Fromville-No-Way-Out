#include "engine/ui/widgets/UILabel.h"
#include "engine/renderer/TextRenderer.h"
#include <glm/vec3.hpp>

UILabel::UILabel(std::string t) : text(std::move(t)) {}

void UILabel::Render(TextRenderer* hud, int screenWidth, int screenHeight) {
    if (!hud || !visible) return;
    hud->RenderText(text, pos.x, pos.y, 1.0f, glm::vec3(0.86f,0.86f,0.86f), screenWidth, screenHeight, false);
}
