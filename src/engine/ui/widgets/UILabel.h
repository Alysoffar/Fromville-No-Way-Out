#pragma once

#include "engine/ui/UIWidget.h"
#include <string>

class TextRenderer;

class UILabel : public UIWidget {
public:
    UILabel(std::string text);
    void SetText(std::string t) { text = std::move(t); }
    void Render(TextRenderer* hud, int screenWidth, int screenHeight) override;

private:
    std::string text;
};
