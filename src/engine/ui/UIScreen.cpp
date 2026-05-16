#include "engine/ui/UIScreen.h"
#include "engine/ui/UIWidget.h"
#include "engine/input/InputContext.h"
#include "engine/input/InputAction.h"

#include "engine/ui/widgets/UIButton.h"

#include <vector>

class FocusableEntry {
public:
    UIWidget* widget = nullptr;
    bool focusable = false;
};

// Basic focus/navigation helpers that UIScreen subclasses can use.
// To keep this lightweight, we provide helper functions rather than a heavy manager.

static float kNavRepeatDelay = 0.18f;

void DefaultNavigate(UIScreen* screen, std::vector<UIWidget*>& items, InputContext& input, float dt, int& selectedIndex) {
    static float repeat = 0.0f;
    repeat += dt;
    if (input.IsActionPressed(InputAction::NavigateUp) || input.IsActionPressed(InputAction::DialogueChoicePrev) || input.IsActionPressed(InputAction::PuzzleUp)) {
        if (repeat > kNavRepeatDelay) {
            selectedIndex = std::max(0, selectedIndex - 1);
            repeat = 0.0f;
        }
    }
    if (input.IsActionPressed(InputAction::NavigateDown) || input.IsActionPressed(InputAction::DialogueChoiceNext) || input.IsActionPressed(InputAction::PuzzleDown)) {
        if (repeat > kNavRepeatDelay) {
            selectedIndex = std::min<int>(static_cast<int>(items.size()) - 1, selectedIndex + 1);
            repeat = 0.0f;
        }
    }

    // apply focus states
    for (size_t i = 0; i < items.size(); ++i) {
        items[i]->SetFocus(static_cast<int>(i) == selectedIndex);
        if (auto* btn = dynamic_cast<UIButton*>(items[i])) {
            btn->SetSelected(static_cast<int>(i) == selectedIndex);
        }
    }
}
