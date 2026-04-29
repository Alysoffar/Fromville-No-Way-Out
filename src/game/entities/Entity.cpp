#include "game/entities/Entity.h"
#include <utility>

Entity::Entity(std::string entityName)
    : name(std::move(entityName)) {
}

const std::string& Entity::GetName() const {
    return name;
}

void Entity::Move(float dirX, float dirZ, float dt) {
    // Pick the right speed based on current state
    float currentSpeed = moveSpeed;
    if (isCrouching) {
        currentSpeed = crouchSpeed;
    } else if (isSprinting) {
        currentSpeed = sprintSpeed;
    }
    
    // Update X and Z positions based on input direction and delta time
    transform.position.x += dirX * currentSpeed * dt;
    transform.position.z += dirZ * currentSpeed * dt;
}

void Entity::Jump() {
    // You can only jump if you are touching the ground and not crouching
    if (isGrounded && !isCrouching) {
        velocityY = jumpForce;
        isGrounded = false;
    }
}

void Entity::Crouch(bool crouch) {
    // Only allow crouching/uncrouching while on the ground
    if (isGrounded) {
        isCrouching = crouch;
        if (isCrouching) isSprinting = false; // Cannot sprint while crouching
    }
}

void Entity::Sprint(bool sprint) {
    // Only allow sprinting if not crouching and on the ground (optional)
    if (!isCrouching) {
        isSprinting = sprint;
    }
}

void Entity::ApplyPhysics(float dt) {
    // 1. Apply gravity to our vertical velocity over time
    if (!isGrounded) {
        velocityY += gravity * dt;
    }

    // 2. Move the entity's Y position based on velocity
    transform.position.y += velocityY * dt;

    // 3. Simple ground collision detection (assuming y = 0 is your floor)
    if (transform.position.y <= 0.0f) {
        transform.position.y = 0.0f;   // Snap to the floor
        velocityY = 0.0f;              // Stop falling
        isGrounded = true;             // We hit the ground
    }
}