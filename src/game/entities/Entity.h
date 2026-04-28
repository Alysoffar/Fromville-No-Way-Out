#pragma once

#include <string>

#include "engine/Transform.h"

class Entity {
public:
    explicit Entity(std::string entityName = "Entity");
    virtual ~Entity() = default;

    virtual void Update(float dt) = 0;

    Transform transform;
    const std::string& GetName() const;

protected:
    std::string name;
};