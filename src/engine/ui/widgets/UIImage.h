#pragma once

#include "engine/ui/UIWidget.h"
#include <string>

class Texture;
class TextRenderer;

class UIImage : public UIWidget {
public:
    UIImage(const std::string& path);
    void Render(TextRenderer* hud, int screenWidth, int screenHeight) override;

private:
    std::string path;
};
