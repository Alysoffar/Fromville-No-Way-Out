#include "game/entities/Entity.h"
#include "engine/physics/CollisionWorld.h"
#include "engine/renderer/AnimatedMesh.h"
#include "engine/renderer/Animation.h"
#include "engine/renderer/Animator.h"
#include "engine/renderer/Shader.h"
#include "engine/renderer/Camera.h"
#include "game/world/DayNightCycle.h"
#include "engine/resources/loader.h"
#include <filesystem>
#include <iostream>
#include <utility>

Entity::Entity(std::string entityName)
    : BaseEntity(std::move(entityName)) {
}

Entity::~Entity() = default;
Entity::Entity(Entity&&) noexcept = default;
Entity& Entity::operator=(Entity&&) noexcept = default;

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

void Entity::Move(float dirX, float dirZ, float dt) {
    // Pick the right speed based on current state
    float currentSpeed = GetMoveSpeed();
    if (isCrouching) {
        currentSpeed = GetCrouchSpeed();
    } else if (isSprinting) {
        currentSpeed = GetSprintSpeed();
    }
    
    // Scale speed down to feel natural in first person perspective
    currentSpeed *= 0.6f;
    
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
    // Calculate horizontal speed
    glm::vec3 moved = transform.position - lastFramePosition;
    moved.y = 0.0f;
    currentSpeed = (dt > 0.0001f) ? (glm::length(moved) / dt) : 0.0f;
    lastFramePosition = transform.position;

    // Update animations based on speed
    if (animator && meshLoaded) {
        UpdateAnimation(dt);
    }

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

    // 2. Build vertical movement delta
    glm::vec3 verticalDelta(0.0f, velocityY * dt, 0.0f);

    if (collisionWorld) {
        // Resolve vertical movement through the collision system.
        // Pass dt=1.0 because delta is already pre-scaled.
        transform.position = collisionWorld->ResolveMovement(
            transform.position, verticalDelta, localBounds, 1.0f);

        // Check grounded state via the collision world
        isGrounded = collisionWorld->IsGrounded(GetWorldAABB(), 0.05f);
        
        // Hardcoded flat terrain floor fallback at Y=0.0 when not on a building structure
        if (!isGrounded && transform.position.y <= 0.0f) {
            transform.position.y = 0.0f;
            isGrounded = true;
        }

        if (isGrounded && velocityY < 0.0f) {
            velocityY = 0.0f;
        }
    } else {
        // Fallback: raw vertical movement with hardcoded floor at y=0
        transform.position.y += velocityY * dt;

        if (transform.position.y <= 0.0f) {
            transform.position.y = 0.0f;
            velocityY = 0.0f;
            isGrounded = true;
            coyoteTimeRemaining = GetCoyoteTime();
        } else {
            isGrounded = false;
        }
    }

    if (isGrounded) {
        coyoteTimeRemaining = GetCoyoteTime();
    }

    TryConsumeJump();
}

void Entity::UpdateAnimation(float dt) {
    (void)dt;
}

void Entity::LoadMesh(const std::string& fbxPath, const std::string& animFbxPath) {
    if (fbxPath == lastLoadedAnimationPath && meshLoaded) {
        return;
    }

    meshLoaded = false;
    lastLoadedAnimationPath = fbxPath;

    try {
        if (!std::filesystem::exists(fbxPath)) {
            throw std::runtime_error("FBX mesh file does not exist: " + fbxPath);
        }

        mesh = std::make_unique<AnimatedMesh>(fbxPath);
        if (!mesh || !mesh->IsValid()) {
            throw std::runtime_error("Failed to construct or initialize AnimatedMesh for " + fbxPath);
        }

        // Load walk animation
        if (!std::filesystem::exists(animFbxPath)) {
            throw std::runtime_error("Animation FBX file does not exist: " + animFbxPath);
        }
        walkingAnimation = std::make_unique<Animation>(animFbxPath, mesh.get());

        // Attempt to load run animation
        std::string runFbxPath = animFbxPath;
        size_t walkPos = runFbxPath.find("Walking.fbx");
        if (walkPos != std::string::npos) {
            runFbxPath.replace(walkPos, 11, "Running.fbx");
        } else {
            walkPos = runFbxPath.find("Walking");
            if (walkPos != std::string::npos) {
                runFbxPath.replace(walkPos, 7, "Running");
            } else {
                walkPos = runFbxPath.find("walk.fbx");
                if (walkPos != std::string::npos) {
                    runFbxPath.replace(walkPos, 8, "run.fbx");
                } else {
                    walkPos = runFbxPath.find("walk");
                    if (walkPos != std::string::npos) {
                        runFbxPath.replace(walkPos, 4, "run");
                    }
                }
            }
        }
        if (std::filesystem::exists(runFbxPath)) {
            try {
                runningAnimation = std::make_unique<Animation>(runFbxPath, mesh.get());
            } catch (...) {
                runningAnimation = nullptr;
            }
        }

        // Attempt to load idle animation
        std::string idleFbxPath = animFbxPath;
        walkPos = idleFbxPath.find("Walking.fbx");
        if (walkPos != std::string::npos) {
            idleFbxPath.replace(walkPos, 11, "Idle.fbx");
        } else {
            walkPos = idleFbxPath.find("Walking");
            if (walkPos != std::string::npos) {
                idleFbxPath.replace(walkPos, 7, "Idle");
            } else {
                walkPos = idleFbxPath.find("walk.fbx");
                if (walkPos != std::string::npos) {
                    idleFbxPath.replace(walkPos, 8, "idle.fbx");
                } else {
                    walkPos = idleFbxPath.find("walk");
                    if (walkPos != std::string::npos) {
                        idleFbxPath.replace(walkPos, 4, "idle");
                    }
                }
            }
        }
        if (std::filesystem::exists(idleFbxPath)) {
            try {
                idleAnimation = std::make_unique<Animation>(idleFbxPath, mesh.get());
            } catch (...) {
                idleAnimation = nullptr;
            }
        }

        animator = std::make_unique<Animator>(walkingAnimation.get(), !IsEnemy());
        
        shader = std::make_unique<Shader>("animated");
        shader->Load("shaders/animated.vert", "shaders/animated.frag");

        meshLoaded = true;
        std::cout << "[LoadMesh] Successfully loaded 3D animated mesh for " << name << " (" << fbxPath << ")\n";
    } catch (const std::exception& e) {
        std::cerr << "[LoadMesh] Failed to load 3D animated mesh for " << name << ": " << e.what() << ". Falling back to 3D cube.\n";
        mesh.reset();
        walkingAnimation.reset();
        runningAnimation.reset();
        idleAnimation.reset();
        animator.reset();
        shader.reset();
        meshLoaded = false;
    }
}

void Entity::RenderMesh(const Camera& camera, float aspectRatio, const DayNightCycle& dayNight, float fogDensity) {
    if (meshLoaded && mesh && shader && animator) {
        shader->Bind();
        shader->SetMat4("projection", camera.GetProjectionMatrix(aspectRatio));
        shader->SetMat4("view", camera.GetViewMatrix());

        glm::mat4 model = glm::translate(glm::mat4(1.0f), transform.position);
        model = glm::rotate(model, glm::radians(transform.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.01f));
        shader->SetMat4("model", model);

        shader->SetVec3("lightDir", dayNight.getActiveLightDir());
        shader->SetVec3("lightColor", dayNight.getLightColor());
        shader->SetVec3("viewPos", camera.GetPosition());
        shader->SetFloat("fogDensity", fogDensity);
        shader->SetVec3("fogColor", dayNight.getFogColor());

        shader->SetBool("uHasTexture", false);
        shader->SetVec3("uFlatColor", GetDebugColor());

        std::vector<glm::mat4> boneMatrices(100, glm::mat4(1.0f));
        shader->SetMat4Array("finalBonesMatrices", boneMatrices);

        mesh->draw(shader->GetID());
        shader->Unbind();
    } else {
        // Fallback debug cube rendering
        static Mesh fallbackMesh;
        static Shader* fallbackShader = nullptr;
        static bool fallbackInitialized = false;

        if (!fallbackInitialized) {
            fallbackShader = new Shader("FallbackEntityCube");
            fallbackShader->Load("assets/shaders/model_lit.vert", "assets/shaders/model_lit.frag");

            // Create colored cube vertices
            std::vector<MeshVertex> vertices = {
                // Front
                {glm::vec3(-0.5f, -0.5f,  0.5f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(1.0f)},
                {glm::vec3( 0.5f, -0.5f,  0.5f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(1.0f)},
                {glm::vec3( 0.5f,  0.5f,  0.5f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(1.0f)},
                {glm::vec3(-0.5f,  0.5f,  0.5f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(1.0f)},
                // Back
                {glm::vec3( 0.5f, -0.5f, -0.5f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(1.0f)},
                {glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(1.0f)},
                {glm::vec3(-0.5f,  0.5f, -0.5f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(1.0f)},
                {glm::vec3( 0.5f,  0.5f, -0.5f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(1.0f)},
                // Left
                {glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(1.0f)},
                {glm::vec3(-0.5f, -0.5f,  0.5f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(1.0f)},
                {glm::vec3(-0.5f,  0.5f,  0.5f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(1.0f)},
                {glm::vec3(-0.5f,  0.5f, -0.5f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(1.0f)},
                // Right
                {glm::vec3( 0.5f, -0.5f,  0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(1.0f)},
                {glm::vec3( 0.5f, -0.5f, -0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(1.0f)},
                {glm::vec3( 0.5f,  0.5f, -0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(1.0f)},
                {glm::vec3( 0.5f,  0.5f,  0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(1.0f)},
                // Top
                {glm::vec3(-0.5f,  0.5f,  0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f)},
                {glm::vec3( 0.5f,  0.5f,  0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f)},
                {glm::vec3( 0.5f,  0.5f, -0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f)},
                {glm::vec3(-0.5f,  0.5f, -0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f)},
                // Bottom
                {glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(1.0f)},
                {glm::vec3( 0.5f, -0.5f, -0.5f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(1.0f)},
                {glm::vec3( 0.5f, -0.5f,  0.5f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(1.0f)},
                {glm::vec3(-0.5f, -0.5f,  0.5f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(1.0f)}
            };

            std::vector<unsigned int> indices = {
                0, 1, 2, 2, 3, 0,
                4, 5, 6, 6, 7, 4,
                8, 9, 10, 10, 11, 8,
                12, 13, 14, 14, 15, 12,
                16, 17, 18, 18, 19, 16,
                20, 21, 22, 22, 23, 20
            };

            fallbackMesh.Create(vertices, indices);
            fallbackInitialized = true;
        }

        fallbackShader->Bind();
        fallbackShader->SetMat4("projection", camera.GetProjectionMatrix(aspectRatio));
        fallbackShader->SetMat4("view", camera.GetViewMatrix());

        // Simple scale mapping for different characters
        glm::vec3 size(0.4f);
        if (name == "Mara" || name == "Elena" || name == "Tom") {
            size = glm::vec3(0.50f, 0.90f, 0.50f);
        } else if (name == "Monster") {
            size = glm::vec3(0.60f, 1.05f, 0.60f);
        }
        
        glm::mat4 model = glm::translate(glm::mat4(1.0f), transform.position + glm::vec3(0.0f, size.y * 0.5f, 0.0f)) * glm::scale(glm::mat4(1.0f), size);
        fallbackShader->SetMat4("model", model);
        fallbackShader->SetVec3("color", GetDebugColor());
        fallbackMesh.Draw();
        fallbackShader->Unbind();
    }
}
