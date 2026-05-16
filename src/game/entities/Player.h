#pragma once

#include "game/entities/Character.h"

class Player : public Character {
public:
    Player();

    void Update(float dt) override;
};