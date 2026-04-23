#pragma once

#include <glm/glm.hpp>

struct PostFXState {
    float desaturation = 0.0f;
    float vignetteStrength = 0.3f;
    float screenShakeIntensity = 0.0f;
    float redChromaticAberration = 0.0f;
    float symbolSightStrength = 0.0f;
    float uvRippleStrength = 0.0f;
    bool isUnderground = false;
    glm::vec3 colorGrade = glm::vec3(1.0f);
};
