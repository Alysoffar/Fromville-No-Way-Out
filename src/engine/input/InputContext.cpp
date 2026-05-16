#include "engine/input/InputContext.h"
#include <algorithm>
#include <utility>
#include <GLFW/glfw3.h>

namespace {
template <typename Predicate>
bool AnyBoundKeyMatches(const KeyBindings& bindings, InputAction action, Predicate&& predicate) {
    const auto& keys = bindings.GetBinding(action);
    return std::any_of(keys.begin(), keys.end(), std::forward<Predicate>(predicate));
}
}  // namespace

InputContext::InputContext(std::string name)
    : name(std::move(name)) {
}

void InputContext::ConfigureGameplayDefaults() {
    SetBinding(InputAction::MoveForward, {GLFW_KEY_W, GLFW_KEY_UP});
    SetBinding(InputAction::MoveBackward, {GLFW_KEY_S, GLFW_KEY_DOWN});
    SetBinding(InputAction::MoveLeft, {GLFW_KEY_A, GLFW_KEY_LEFT});
    SetBinding(InputAction::MoveRight, {GLFW_KEY_D, GLFW_KEY_RIGHT});
    SetBinding(InputAction::Sprint, {GLFW_KEY_LEFT_SHIFT});
    SetBinding(InputAction::Crouch, {GLFW_KEY_C});
    SetBinding(InputAction::Jump, {GLFW_KEY_SPACE});
    SetBinding(InputAction::Interact, {GLFW_KEY_E});
    SetBinding(InputAction::Pickup, {GLFW_KEY_F});
    SetBinding(InputAction::Ability, {GLFW_KEY_Q});
    SetBinding(InputAction::SaveGame, {GLFW_KEY_F5});
    SetBinding(InputAction::LoadGame, {GLFW_KEY_F9});
    SetBinding(InputAction::ResetView, {GLFW_KEY_R});
    SetBinding(InputAction::Pause, {GLFW_KEY_ESCAPE});
    SetBinding(InputAction::AbandonQuest, {GLFW_KEY_Q});
    SetBinding(InputAction::SwitchCharacter1, {GLFW_KEY_1});
    SetBinding(InputAction::SwitchCharacter2, {GLFW_KEY_2});
    SetBinding(InputAction::SwitchCharacter3, {GLFW_KEY_3});
    SetBinding(InputAction::SwitchCharacter4, {GLFW_KEY_4});
    SetBinding(InputAction::SwitchCharacter5, {GLFW_KEY_5});
    SetBinding(InputAction::DebugDump, {GLFW_KEY_F12});
    SetBinding(InputAction::DebugToggleUI, {GLFW_KEY_F11});
    SetBinding(InputAction::DebugNext, {GLFW_KEY_PAGE_UP});
    SetBinding(InputAction::DebugPrev, {GLFW_KEY_PAGE_DOWN});
    SetBinding(InputAction::DebugInc, {GLFW_KEY_LEFT_BRACKET});
    SetBinding(InputAction::DebugDec, {GLFW_KEY_RIGHT_BRACKET});
}

void InputContext::ConfigureUiDefaults() {
    SetBinding(InputAction::Confirm, {GLFW_KEY_ENTER, GLFW_KEY_SPACE});
    SetBinding(InputAction::Cancel, {GLFW_KEY_ESCAPE, GLFW_KEY_BACKSPACE});
    SetBinding(InputAction::DialogueChoicePrev, {GLFW_KEY_UP, GLFW_KEY_W});
    SetBinding(InputAction::DialogueChoiceNext, {GLFW_KEY_DOWN, GLFW_KEY_S});
    SetBinding(InputAction::DialogueChoice1, {GLFW_KEY_1});
    SetBinding(InputAction::DialogueChoice2, {GLFW_KEY_2});
    SetBinding(InputAction::DialogueChoice3, {GLFW_KEY_3});
    SetBinding(InputAction::DialogueChoice4, {GLFW_KEY_4});
    SetBinding(InputAction::DialogueAdvance, {GLFW_KEY_ENTER, GLFW_KEY_SPACE});
    SetBinding(InputAction::DialogueCancel, {GLFW_KEY_ESCAPE, GLFW_KEY_BACKSPACE});
    SetBinding(InputAction::OpenJournal, {GLFW_KEY_J});
    SetBinding(InputAction::ToggleHint, {GLFW_KEY_LEFT_CONTROL, GLFW_KEY_RIGHT_CONTROL});
    SetBinding(InputAction::PuzzleReset, {GLFW_KEY_R});
    SetBinding(InputAction::PuzzleConfirm, {GLFW_KEY_ENTER, GLFW_KEY_SPACE});
    SetBinding(InputAction::PuzzleCancel, {GLFW_KEY_ESCAPE, GLFW_KEY_BACKSPACE});
    SetBinding(InputAction::PuzzleOption1, {GLFW_KEY_1});
    SetBinding(InputAction::PuzzleOption2, {GLFW_KEY_2});
    SetBinding(InputAction::PuzzleOption3, {GLFW_KEY_3});
    SetBinding(InputAction::PuzzleOption4, {GLFW_KEY_4});
    SetBinding(InputAction::PuzzleOption5, {GLFW_KEY_5});
    SetBinding(InputAction::PuzzleUp, {GLFW_KEY_W, GLFW_KEY_UP});
    SetBinding(InputAction::PuzzleDown, {GLFW_KEY_S, GLFW_KEY_DOWN});
    SetBinding(InputAction::PuzzleLeft, {GLFW_KEY_A, GLFW_KEY_LEFT});
    SetBinding(InputAction::PuzzleRight, {GLFW_KEY_D, GLFW_KEY_RIGHT});
    SetBinding(InputAction::PuzzleIncrease, {GLFW_KEY_E});
    SetBinding(InputAction::PuzzleDecrease, {GLFW_KEY_Q});
    SetBinding(InputAction::PuzzleMoveForward, {GLFW_KEY_W, GLFW_KEY_UP});
    SetBinding(InputAction::PuzzleMoveBackward, {GLFW_KEY_S, GLFW_KEY_DOWN});
    SetBinding(InputAction::PuzzleMoveLeft, {GLFW_KEY_A, GLFW_KEY_LEFT});
    SetBinding(InputAction::PuzzleMoveRight, {GLFW_KEY_D, GLFW_KEY_RIGHT});
    SetBinding(InputAction::PuzzleSprint, {GLFW_KEY_LEFT_SHIFT});
    SetBinding(InputAction::Pause, {GLFW_KEY_ESCAPE});
}

void InputContext::SetBinding(InputAction action, std::initializer_list<int> keys) {
    bindings.SetBinding(action, keys);
}

void InputContext::ClearBinding(InputAction action) {
    bindings.ClearBinding(action);
}

bool InputContext::IsActionDown(InputAction action) const {
    return IsEnabled() && IsBoundKeyDown(action);
}

bool InputContext::IsActionPressed(InputAction action) const {
    return IsEnabled() && IsBoundKeyPressed(action);
}

bool InputContext::IsActionReleased(InputAction action) const {
    return IsEnabled() && IsBoundKeyReleased(action);
}

float InputContext::GetAxis(InputAction negativeAction, InputAction positiveAction) const {
    const bool negative = IsActionDown(negativeAction);
    const bool positive = IsActionDown(positiveAction);
    return static_cast<float>(positive) - static_cast<float>(negative);
}

glm::vec2 InputContext::GetMovementVector() const {
    glm::vec2 movement(0.0f);
    movement.y += GetAxis(InputAction::MoveBackward, InputAction::MoveForward);
    movement.x += GetAxis(InputAction::MoveLeft, InputAction::MoveRight);

    if (glm::length(movement) > 0.001f) {
        movement = glm::normalize(movement);
    }

    return movement;
}

glm::vec2 InputContext::GetMousePosition() const {
    if (!input) return glm::vec2(0.0f);
    return input->GetMousePos();
}

glm::ivec2 InputContext::GetFramebufferSize() const {
    if (!input) return glm::ivec2(0, 0);
    return input->GetFramebufferSize();
}

bool InputContext::IsMouseButtonDown(int button) const {
    return input && input->IsMouseButtonDown(button);
}

bool InputContext::IsMouseButtonPressed(int button) const {
    return input && input->IsMouseButtonPressed(button);
}

bool InputContext::IsBoundKeyDown(InputAction action) const {
    if (!input) return false;
    return AnyBoundKeyMatches(bindings, action, [this](int key) {
        return input->IsKeyDown(key);
    });
}

bool InputContext::IsBoundKeyPressed(InputAction action) const {
    if (!input) return false;
    return AnyBoundKeyMatches(bindings, action, [this](int key) {
        return input->IsKeyPressed(key);
    });
}

bool InputContext::IsBoundKeyReleased(InputAction action) const {
    if (!input) return false;
    return AnyBoundKeyMatches(bindings, action, [this](int key) {
        return input->IsKeyReleased(key);
    });
}
