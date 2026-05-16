#include "engine/renderer/Camera.h"

#include <algorithm>
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>

#include "engine/core/InputManager.h"

namespace {

static constexpr float DEG2RAD = 3.14159265358979323846f / 180.0f;

} // namespace

Camera::Camera() {
    updateCameraVectors();
}

glm::mat4 Camera::GetViewMatrix() const {
    return glm::lookAt(Position, Position + Front, Up);
}

glm::mat4 Camera::GetProjectionMatrix(float aspect) const {
    return glm::perspective(glm::radians(Zoom), aspect, nearPlane, farPlane);
}

float Camera::wrapDegrees(float deg) {
    // Wrap to [0, 360)
    while (deg < 0.0f) deg += 360.0f;
    while (deg >= 360.0f) deg -= 360.0f;
    return deg;
}

float Camera::lerpAngleDegreesShortest(float aDeg, float bDeg, float t) {
    float from = wrapDegrees(aDeg);
    float to = wrapDegrees(bDeg);
    float delta = to - from;
    if (delta > 180.0f) delta -= 360.0f;
    if (delta < -180.0f) delta += 360.0f;
    return wrapDegrees(from + delta * t);
}

void Camera::Update(InputManager& input, float dt, const glm::vec3& playerPosition) {
    (void)dt;

    if (currentState != CameraState::FIRST_PERSON) {
        updateTransition(dt);
        return;
    }

    // FIRST_PERSON: cinematic input lock is respected
    if (!inputLocked_) {
        const glm::vec2 mouseDelta = input.GetMouseDelta();
        ProcessMouseMovement(mouseDelta.x, -mouseDelta.y, true);
    }

    // FIRST_PERSON strictly locks to player + eye height every frame.
    Position = playerPosition + glm::vec3(0.0f, EYE_HEIGHT, 0.0f);

    // Prevent camera from going under the ground
    if (Position.y < 0.2f) Position.y = 0.2f;

    // First-person should exclude player mesh from rendering to avoid clipping (layer system)
    cullingMask = ~static_cast<uint32_t>(RenderLayer::LAYER_PLAYER);
}

void Camera::ProcessMouseMovement(float xOffset, float yOffset, bool constrainPitch) {
    xOffset *= MouseSensitivity;
    yOffset *= MouseSensitivity;

    Yaw += xOffset;
    Pitch += yOffset;

    if (constrainPitch) {
        Pitch = clampFloat(Pitch, -89.0f, 89.0f);
    }

    updateCameraVectors();
}

void Camera::ProcessMouseScroll(float yOffset) {
    Zoom -= yOffset;
    Zoom = clampFloat(Zoom, 1.0f, 45.0f);
}

void Camera::triggerTransition(const glm::vec3& newTargetPos, float newTargetPitch, float newTargetYaw) {
    // Start pose (already in eye-space while in FIRST_PERSON)
    StartPos = Position;
    StartPitch = Pitch;
    StartYaw = Yaw;

    // Targets are provided as "player/world position" (feet/base). Convert to eye-space.
    TargetPos = newTargetPos + glm::vec3(0.0f, EYE_HEIGHT, 0.0f);

    newTargetPitch_ = newTargetPitch;
    newTargetYaw_ = newTargetYaw;

    transitionTimer = 0.0f;
    currentState = CameraState::TRANSITION_RISING;

    // During transitions, we should show player (keep default rendering)
    cullingMask = static_cast<uint32_t>(RenderLayer::LAYER_DEFAULT) |
                   static_cast<uint32_t>(RenderLayer::LAYER_PLAYER);

    inputLocked_ = true;
}

void Camera::updateTransition(float deltaTime) {
    if (currentState == CameraState::FIRST_PERSON) {
        return;
    }

    transitionTimer += deltaTime;

    const float tRaw = (phaseDuration > 0.0f) ? (transitionTimer / phaseDuration) : 1.0f;
    const float t = Camera::smoothstep(0.0f, 1.0f, tRaw);

    switch (currentState) {
        case CameraState::TRANSITION_RISING: {
            // Eye-space rising: keep Y as part of interpolation (no ground clipping dip)
            Position = glm::mix(StartPos, StartPos + glm::vec3(0.0f, maxAltitude, 0.0f), t);

            // Pitch pans downward to -89
            Pitch = glm::mix(StartPitch, -89.0f, t);

            updateCameraVectors();

            if (tRaw >= 1.0f) {
                currentState = CameraState::TRANSITION_PANNING;
                transitionTimer = 0.0f;
            }
            break;
        }

        case CameraState::TRANSITION_PANNING: {
            // Eye-space panning at high altitude
            const glm::vec3 highStart = StartPos + glm::vec3(0.0f, maxAltitude, 0.0f);
            const glm::vec3 highEnd = TargetPos + glm::vec3(0.0f, maxAltitude, 0.0f);

            Position = glm::mix(highStart, highEnd, t);

            // Pitch stays at -89 during panning
            Pitch = -89.0f;

            updateCameraVectors();

            if (tRaw >= 1.0f) {
                currentState = CameraState::TRANSITION_DESCENDING;
                transitionTimer = 0.0f;
            }
            break;
        }

        case CameraState::TRANSITION_DESCENDING: {
            // Eye-space descending ends at eye-height TargetPos (already offset in triggerTransition)
            const glm::vec3 highStart = TargetPos + glm::vec3(0.0f, maxAltitude, 0.0f);

            Position = glm::mix(highStart, TargetPos, t);
            Pitch = glm::mix(-89.0f, newTargetPitch_, t);

            // Yaw shortest-path interpolation (wrap-safe)
            Yaw = lerpAngleDegreesShortest(StartYaw, newTargetYaw_, t);

            updateCameraVectors();

            if (tRaw >= 1.0f) {
                currentState = CameraState::FIRST_PERSON;
                inputLocked_ = false;
            }
            break;
        }

        case CameraState::FIRST_PERSON:
        default:
            break;
    }
}

void Camera::processKeyboard(int direction, float deltaTime) {
    if (currentState != CameraState::FIRST_PERSON) {
        return;
    }

    const float velocity = MovementSpeed * deltaTime;

    // 0: forward, 1: backward, 2: left, 3: right
    switch (direction) {
        case 0: Position += Front * velocity; break;
        case 1: Position -= Front * velocity; break;
        case 2: Position -= Right * velocity; break;
        case 3: Position += Right * velocity; break;
        default: break;
    }

    if (Position.y < 0.2f) Position.y = 0.2f;
}

glm::vec3 Camera::GetPosition() const {
    return Position;
}

glm::vec3 Camera::GetForward() const {
    return Front;
}

glm::vec3 Camera::GetRight() const {
    return Right;
}

glm::vec3 Camera::GetUp() const {
    return Up;
}

void Camera::SetRotation(float newYaw, float newPitch) {
    Yaw = newYaw;
    Pitch = clampFloat(newPitch, -89.0f, 89.0f);
    updateCameraVectors();
}

void Camera::Reset(const glm::vec3& spawnPosition) {
    Position = spawnPosition;
    WorldUp = glm::vec3(0.0f, 1.0f, 0.0f);

    Yaw = -90.0f;
    Pitch = 0.0f;

    Zoom = 60.0f;
    nearPlane = 0.1f;
    farPlane = 500.0f;

    StartPos = glm::vec3(0.0f);
    TargetPos = glm::vec3(0.0f);
    StartPitch = 0.0f;
    StartYaw = 0.0f;

    transitionTimer = 0.0f;
    currentState = CameraState::FIRST_PERSON;
    inputLocked_ = false;

    updateCameraVectors();

    // Default to exclude player mesh in first-person
    cullingMask = ~static_cast<uint32_t>(RenderLayer::LAYER_PLAYER);
}

void Camera::updateCameraVectors() {
    glm::vec3 front;
    front.x = std::cos(Yaw * DEG2RAD) * std::cos(Pitch * DEG2RAD);
    front.y = std::sin(Pitch * DEG2RAD);
    front.z = std::sin(Yaw * DEG2RAD) * std::cos(Pitch * DEG2RAD);

    Front = glm::normalize(front);
    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up = glm::normalize(glm::cross(Right, Front));
}
