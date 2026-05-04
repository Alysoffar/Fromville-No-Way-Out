#pragma once

#include <glm/glm.hpp>
#include "engine/physics/Collision.h"

struct Triangle {
    glm::vec3 a, b, c;

    AABB GetBounds() const {
        AABB bounds;
        bounds.min = glm::min(a, glm::min(b, c));
        bounds.max = glm::max(a, glm::max(b, c));
        return bounds;
    }

    glm::vec3 GetCenter() const {
        return (a + b + c) / 3.0f;
    }
};
