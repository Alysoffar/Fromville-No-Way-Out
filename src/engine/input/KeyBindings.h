#pragma once

#include <array>
#include <initializer_list>
#include <vector>

#include "engine/input/InputAction.h"

class KeyBindings {
public:
    void SetBinding(InputAction action, std::initializer_list<int> keys);
    void ClearBinding(InputAction action);
    const std::vector<int>& GetBinding(InputAction action) const;

private:
    std::array<std::vector<int>, kInputActionCount> bindings;
};
