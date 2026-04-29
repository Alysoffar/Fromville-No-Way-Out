#include "game/entities/Enemy.h"

Enemy::Enemy()
    : Entity("Enemy") {
}

void Enemy::Update(float dt) {
    ApplyPhysics(dt);
}