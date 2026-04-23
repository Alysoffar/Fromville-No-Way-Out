#pragma once

#include <string>
#include <glm/glm.hpp>

class NPC {
public:
    int id = 0;
    std::string name;
    float health = 100.0f;
    glm::vec3 position = glm::vec3(0.0f);
    bool alive = true;

    void Update(float dt) {}
};
