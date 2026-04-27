#include "game/entities/Enemy.h"

Enemy::Enemy()
    : Entity("Enemy") {
}

void Enemy::Update(float dt) {
    (void)dt;
    (void)health;
}