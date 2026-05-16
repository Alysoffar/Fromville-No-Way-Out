#pragma once

#include <string>
#include <glm/glm.hpp>

#include "engine/core/InputManager.h"
#include "engine/input/InputAction.h"
#include "engine/input/KeyBindings.h"

class InputContext {
public:
    explicit InputContext(std::string name = "Gameplay");

    void SetInputManager(InputManager* manager) { input = manager; }
    void Attach(InputManager* manager) { input = manager; }
    void SetEnabled(bool enabled) { this->enabled = enabled; }
    bool IsEnabled() const { return enabled; }
    const std::string& GetName() const { return name; }
    const InputManager* GetInputManager() const { return input; }

    void ConfigureGameplayDefaults();
    void ConfigureUiDefaults();

    void SetBinding(InputAction action, std::initializer_list<int> keys);
    void ClearBinding(InputAction action);

    bool IsActionDown(InputAction action) const;
    bool IsActionPressed(InputAction action) const;
    bool IsActionReleased(InputAction action) const;
    float GetAxis(InputAction negativeAction, InputAction positiveAction) const;
    glm::vec2 GetMovementVector() const;
    glm::vec2 GetMousePosition() const;
    glm::ivec2 GetFramebufferSize() const;
    bool IsMouseButtonDown(int button) const;
    bool IsMouseButtonPressed(int button) const;

private:
    std::string name;
    InputManager* input = nullptr;
    bool enabled = true;
    KeyBindings bindings;

    bool IsBoundKeyDown(InputAction action) const;
    bool IsBoundKeyPressed(InputAction action) const;
    bool IsBoundKeyReleased(InputAction action) const;
};
