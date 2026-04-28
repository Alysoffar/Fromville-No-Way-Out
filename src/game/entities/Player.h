#pragma once

#include "game/entities/Entity.h"

class Player : public Entity {
public:
    Player();

    void Update(float dt) override;
};