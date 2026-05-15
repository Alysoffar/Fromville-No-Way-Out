#pragma once

#include "engine/ui/UIWidget.h"

class TextRenderer;
class InputContext;

class UISlider : public UIWidget {
public:
    UISlider(float min = 0.0f, float max = 1.0f);
    void Update(float dt, InputContext& input) override;
    void Render(TextRenderer* hud, int screenWidth, int screenHeight) override;

    void SetValue(float v) { value = v; }
    float GetValue() const { return value; }

private:
    float minValue;
    float maxValue;
    float value;
};
