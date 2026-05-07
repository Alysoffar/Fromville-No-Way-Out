#pragma once

#include <string>

#include <glm/glm.hpp>

#include "engine/Transform.h"
#include "engine/physics/Collision.h"

class CollisionWorld;

class Entity {
public:
    explicit Entity(std::string entityName = "Entity");
    virtual ~Entity() = default;

    virtual void Update(float dt) = 0;

    // Movement functions that derived classes can call
    virtual void Move(float dirX, float dirZ, float dt);
    virtual void Jump();
    virtual void ReleaseJump();
    virtual void Crouch(bool crouch);
    virtual void Sprint(bool sprint);
    virtual glm::vec3 GetDebugColor() const;
    bool IsCrouching() const { return isCrouching; }
    bool IsSprinting() const { return isSprinting; }

    Transform transform;
    const std::string& GetName() const;

protected:
    std::string name;

    // Movement profile hooks for derived character types.
    virtual float GetMoveSpeed() const;
    virtual float GetSprintSpeed() const;
    virtual float GetCrouchSpeed() const;
    virtual float GetJumpForce() const;
    virtual float GetGravity() const;
    virtual float GetFallGravityMultiplier() const;
    virtual float GetMaxFallSpeed() const;
    virtual float GetJumpBufferTime() const;
    virtual float GetCoyoteTime() const;
    virtual float GetJumpCutMultiplier() const;

    // Current state variables
    float velocityY = 0.0f;
    bool isGrounded = true;
    bool isCrouching = false;
    bool isSprinting = false;
    float jumpBufferRemaining = 0.0f;
    float coyoteTimeRemaining = 0.0f;

    // Collision system
    CollisionWorld* collisionWorld = nullptr;
    AABB localBounds = { glm::vec3(-0.4f, 0.0f, -0.4f), glm::vec3(0.4f, 1.8f, 0.4f) };

    AABB GetWorldAABB() const {
        return AABB {
            localBounds.min + transform.position,
            localBounds.max + transform.position
        };
    }

public:
    void SetCollisionWorld(CollisionWorld* cw) { collisionWorld = cw; }

protected:
    // Helper function to handle gravity and falling
    void ApplyPhysics(float dt);
    void TryConsumeJump();
};
