#include "engine/ui/widgets/UIImage.h"
#include "engine/renderer/TextRenderer.h"

UIImage::UIImage(const std::string& p) : path(p) {}

void UIImage::Render(TextRenderer* hud, int screenWidth, int screenHeight) {
    // placeholder: draw a text placeholder for image
    if (!hud || !visible) return;
    hud->RenderText("[image]", pos.x, pos.y, 0.8f, glm::vec3(0.6f,0.6f,0.6f), screenWidth, screenHeight, false);
}
