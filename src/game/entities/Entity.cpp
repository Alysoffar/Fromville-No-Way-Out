#include "game/entities/Entity.h"
#include "engine/physics/CollisionWorld.h"
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

    // Build 3D horizontal delta
    glm::vec3 delta(dirX * currentSpeed * dt, 0.0f, dirZ * currentSpeed * dt);

    if (collisionWorld) {
        // ResolveMovement multiplies velocity*deltaTime internally,
        // so pass delta as-is with dt=1.0 to avoid double-scaling
        transform.position = collisionWorld->ResolveMovement(
            transform.position, delta, localBounds, 1.0f);
    } else {
        // Fallback: raw position update
        transform.position += delta;
    }

    // Rotate player to face movement direction
    if (glm::length(glm::vec2(dirX, dirZ)) > 0.001f) {
        transform.rotation.y = glm::degrees(atan2(dirX, dirZ));
    }
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
    // 1. Apply gravity to vertical velocity if in the air
    if (!isGrounded) {
        velocityY += gravity * dt;
    } else if (velocityY < 0.0f) {
        // Reset downward velocity when grounded
        velocityY = 0.0f;
    }

    // 2. Build vertical movement delta
    glm::vec3 verticalDelta(0.0f, velocityY * dt, 0.0f);

    if (collisionWorld) {
        // 3. Resolve vertical movement
        transform.position = collisionWorld->ResolveMovement(
            transform.position, verticalDelta, localBounds, 1.0f);

        // 4. Update grounded state
        isGrounded = collisionWorld->IsGrounded(GetWorldAABB(), 0.05f);
    }

    // Always enforce floor at Y=0 as a safety measure for the flat world
    if (transform.position.y <= 0.001f) {
        transform.position.y = 0.001f;
        if (velocityY < 0.0f) velocityY = 0.0f;
        isGrounded = true;
    } else if (!collisionWorld) {
        isGrounded = false;
    }
}

