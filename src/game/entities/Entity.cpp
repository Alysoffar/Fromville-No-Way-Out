#include "game/entities/Entity.h"
#include <utility>

Entity::Entity(std::string entityName)
    : name(std::move(entityName)) {
}

glm::vec3 Entity::GetDebugColor() const {
    return glm::vec3(0.85f);
}

float Entity::GetMoveSpeed() const {
    return 8.0f;
}

float Entity::GetSprintSpeed() const {
    return 14.0f;
}

float Entity::GetCrouchSpeed() const {
    return 3.0f;
}

float Entity::GetJumpForce() const {
    return 8.0f;
}

float Entity::GetGravity() const {
    return -20.0f;
}

float Entity::GetFallGravityMultiplier() const {
    return 1.6f;
}

float Entity::GetMaxFallSpeed() const {
    return -30.0f;
}

float Entity::GetJumpBufferTime() const {
    return 0.12f;
}

float Entity::GetCoyoteTime() const {
    return 0.10f;
}

float Entity::GetJumpCutMultiplier() const {
    return 0.55f;
}

const std::string& Entity::GetName() const {
    return name;
}

void Entity::Move(float dirX, float dirZ, float dt) {
    // Pick the right speed based on current state
    float currentSpeed = GetMoveSpeed();
    if (isCrouching) {
        currentSpeed = GetCrouchSpeed();
    } else if (isSprinting) {
        currentSpeed = GetSprintSpeed();
    }
    
    // Update X and Z positions based on input direction and delta time
    transform.position.x += dirX * currentSpeed * dt;
    transform.position.z += dirZ * currentSpeed * dt;
}

void Entity::Jump() {
    jumpBufferRemaining = GetJumpBufferTime();
    TryConsumeJump();
}

void Entity::ReleaseJump() {
    if (velocityY > 0.0f) {
        velocityY *= GetJumpCutMultiplier();
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

void Entity::TryConsumeJump() {
    if (jumpBufferRemaining <= 0.0f) {
        return;
    }

    if (!isGrounded && coyoteTimeRemaining <= 0.0f) {
        return;
    }

    if (isCrouching) {
        return;
    }

    velocityY = GetJumpForce();
    isGrounded = false;
    jumpBufferRemaining = 0.0f;
    coyoteTimeRemaining = 0.0f;
}

void Entity::ApplyPhysics(float dt) {
    if (jumpBufferRemaining > 0.0f) {
        jumpBufferRemaining -= dt;
        if (jumpBufferRemaining < 0.0f) {
            jumpBufferRemaining = 0.0f;
        }
    }

    // 1. Apply gravity to our vertical velocity over time
    if (!isGrounded) {
        if (coyoteTimeRemaining > 0.0f) {
            coyoteTimeRemaining -= dt;
            if (coyoteTimeRemaining < 0.0f) {
                coyoteTimeRemaining = 0.0f;
            }
        }

        const float gravityScale = (velocityY > 0.0f) ? 1.0f : GetFallGravityMultiplier();
        velocityY += GetGravity() * gravityScale * dt;
        if (velocityY < GetMaxFallSpeed()) {
            velocityY = GetMaxFallSpeed();
        }
    }

    // 2. Move the entity's Y position based on velocity
    transform.position.y += velocityY * dt;

    // 3. Simple ground collision detection (assuming y = 0 is your floor)
    if (transform.position.y <= 0.0f) {
        transform.position.y = 0.0f;   // Snap to the floor
        velocityY = 0.0f;              // Stop falling
        isGrounded = true;             // We hit the ground
        coyoteTimeRemaining = GetCoyoteTime();
    } else {
        isGrounded = false;
    }

    TryConsumeJump();

    if (isGrounded) {
        coyoteTimeRemaining = GetCoyoteTime();
    }
}