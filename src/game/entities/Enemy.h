#pragma once

#include "game/entities/Entity.h"

class Enemy : public Entity {
public:
    Enemy();

    void Update(float dt) override;

private:
    float health = 100.0f;
};