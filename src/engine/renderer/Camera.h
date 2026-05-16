#pragma once

#include <cstdint>
#include <glm/glm.hpp>

class InputManager;

class Camera {
public:
    enum class CameraState {
        FIRST_PERSON,
        TRANSITION_RISING,
        TRANSITION_PANNING,
        TRANSITION_DESCENDING
    };

    // Render layers (bitmask). Keep in sync with places that do culling checks.
    enum RenderLayer : uint32_t {
        LAYER_DEFAULT = 1u << 0,
        LAYER_PLAYER  = 1u << 1,
        LAYER_SHADOW  = 1u << 2,
        LAYER_UI      = 1u << 3
    };

    Camera();

    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix(float aspect) const;

    void Update(InputManager& input, float dt, const glm::vec3& playerPosition);

    // Mouse / scroll APIs (scroll optional for integration)
    void ProcessMouseMovement(float xOffset, float yOffset, bool constrainPitch = true);
    void ProcessMouseScroll(float yOffset);

    // Transition state machine
    void triggerTransition(const glm::vec3& newTargetPos, float newTargetPitch, float newTargetYaw);
    void updateTransition(float deltaTime);

    // Movement processing (only runs when FIRST_PERSON)
    void processKeyboard(int direction, float deltaTime);

    // Accessors used by renderer / gameplay
    glm::vec3 GetPosition() const;
    glm::vec3 GetForward() const;
    glm::vec3 GetRight() const;
    glm::vec3 GetUp() const;

    void SetRotation(float newYaw, float newPitch);
    void Reset(const glm::vec3& spawnPosition);

    CameraState GetState() const { return currentState; }

    // Culling mask: render code should skip meshes whose (meshLayer & cullingMask)==0
    uint32_t cullingMask{0xFFFFFFFFu};

private:
    static constexpr float EYE_HEIGHT = 1.8f;

    // First-person camera vectors + parameters
    glm::vec3 Position{0.0f, EYE_HEIGHT, 0.0f};
    glm::vec3 Front{};
    glm::vec3 Up{};
    glm::vec3 Right{};
    glm::vec3 WorldUp{0.0f, 1.0f, 0.0f};

    float Yaw{-90.0f};
    float Pitch{0.0f};
    float MovementSpeed{4.5f};
    float MouseSensitivity{0.08f};
    float Zoom{60.0f};

    float nearPlane{0.1f};
    float farPlane{500.0f};

    CameraState currentState{CameraState::FIRST_PERSON};

    // Transition parameters / state
    glm::vec3 StartPos{};
    glm::vec3 TargetPos{};

    float StartPitch{0.0f};
    float StartYaw{0.0f};

    float transitionTimer{0.0f};
    float maxAltitude{300.0f};
    float phaseDuration{2.0f}; // seconds per phase

    // New target for descending pitch/yaw
    float newTargetPitch_{0.0f};
    float newTargetYaw_{0.0f};

    bool inputLocked_{false};

    void updateCameraVectors();

    static float clampFloat(float v, float lo, float hi) {
        return v < lo ? lo : (v > hi ? hi : v);
    }

    static float smoothstep(float edge0, float edge1, float x) {
        float t = clampFloat((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

    static float lerpAngleDegreesShortest(float aDeg, float bDeg, float t);

    static float wrapDegrees(float deg);
};
