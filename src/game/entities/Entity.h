#pragma once

#include <string>
#include <memory>

#include "engine/physics/Collision.h"
#include "game/entities/BaseEntity.h"

class CollisionWorld;
class AnimatedMesh;
class Animation;
class Animator;
class Shader;
class Camera;
class DayNightCycle;

class Entity : public BaseEntity {
public:
    explicit Entity(std::string entityName = "Entity");
    virtual ~Entity();
    Entity(Entity&&) noexcept;
    Entity& operator=(Entity&&) noexcept;

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
    void SetCurrentSpeed(float speed) { currentSpeed = speed; }
    float GetCurrentSpeed() const { return currentSpeed; }
    virtual bool IsEnemy() const { return false; }
    virtual void UpdateAnimation(float dt);

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

    // 3D animated mesh pipeline methods
    void LoadMesh(const std::string& fbxPath, const std::string& animFbxPath);
    void RenderMesh(const Camera& camera, float aspectRatio, const DayNightCycle& dayNight, float fogDensity);

protected:
    // Current state variables
    float velocityY = 0.0f;
    bool isGrounded = true;
    bool isCrouching = false;
    bool isSprinting = false;
    float jumpBufferRemaining = 0.0f;
    float coyoteTimeRemaining = 0.0f;
    float currentSpeed = 0.0f;

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

    // 3D animated mesh rendering variables
    std::unique_ptr<AnimatedMesh> mesh;
    std::unique_ptr<Animation> walkingAnimation;
    std::unique_ptr<Animation> runningAnimation;
    std::unique_ptr<Animation> idleAnimation;
    std::unique_ptr<Animator> animator;
    std::unique_ptr<Shader> shader;
    bool meshLoaded = false;
    float animSpeedMultiplier = 1.0f;
    std::string lastLoadedAnimationPath;
    glm::vec3 lastFramePosition = glm::vec3(0.0f);
};