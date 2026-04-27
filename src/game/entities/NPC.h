#pragma once

#include "game/entities/Entity.h"

class NPC : public Entity {
public:
    explicit NPC(std::string displayName = "NPC");
    virtual ~NPC() = default;

    virtual void Update(float dt);
};