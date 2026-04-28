#pragma once

#include <string>

#include <glm/glm.hpp>

#include "engine/Transform.h"

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

    // Helper function to handle gravity and falling
    void ApplyPhysics(float dt);
    void TryConsumeJump();
};