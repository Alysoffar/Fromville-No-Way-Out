#pragma once

#include <glm/glm.hpp>

struct HitResult {
    glm::vec3 point = glm::vec3(0.0f);
    glm::vec3 normal = glm::vec3(0.0f, 1.0f, 0.0f);
    float t = 1.0f; // Time/distance of hit (0 to 1 for sweeps/rays)
    bool hit = false;
};
