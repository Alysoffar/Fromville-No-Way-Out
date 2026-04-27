#pragma once

#include <glm/glm.hpp>

struct AABB {
    glm::vec3 min = glm::vec3(0.0f);
    glm::vec3 max = glm::vec3(0.0f);

    bool Intersects(const AABB& other) const;
};