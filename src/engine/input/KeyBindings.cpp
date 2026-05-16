#include "engine/input/KeyBindings.h"

void KeyBindings::SetBinding(InputAction action, std::initializer_list<int> keys) {
    bindings[ToIndex(action)] = std::vector<int>(keys.begin(), keys.end());
}

void KeyBindings::ClearBinding(InputAction action) {
    bindings[ToIndex(action)].clear();
}

const std::vector<int>& KeyBindings::GetBinding(InputAction action) const {
    return bindings[ToIndex(action)];
}
