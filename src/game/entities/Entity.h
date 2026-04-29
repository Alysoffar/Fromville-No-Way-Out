#pragma once

#include <string>
#include "engine/Transform.h"

class Entity {
public:
    explicit Entity(std::string entityName = "Entity");
    virtual ~Entity() = default;

    virtual void Update(float dt) = 0;

    // Movement functions that derived classes can call
    virtual void Move(float dirX, float dirZ, float dt);
    virtual void Jump();
    virtual void Crouch(bool crouch);
    virtual void Sprint(bool sprint);

    Transform transform;
    const std::string& GetName() const;

protected:
    std::string name;

    // Physics & Movement parameters
    float moveSpeed = 8.0f;
    float sprintSpeed = 14.0f;
    float crouchSpeed = 3.0f;
    float jumpForce = 8.0f;
    float gravity = -20.0f;

    // Current state variables
    float velocityY = 0.0f;
    bool isGrounded = true;
    bool isCrouching = false;
    bool isSprinting = false;

    // Helper function to handle gravity and falling
    void ApplyPhysics(float dt);
};