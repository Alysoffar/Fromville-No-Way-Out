#include "engine/ui/MainMenuScreen.h"
#include "engine/renderer/TextRenderer.h"
#include "engine/input/InputContext.h"

#include <glm/vec3.hpp>
#include <algorithm>
#include <cmath>
#include "engine/ui/widgets/UIButton.h"
#include "engine/ui/TransitionManager.h"

using namespace std;

MainMenuScreen::MainMenuScreen() {
    items = {"Continue", "New Game", "Load Game", "Settings", "Credits", "Quit"};
}

void MainMenuScreen::OnEnter() {
    selected = 0;
    blinkTimer = 0.0f;
    // create button widgets
    // positions are rough; this will be refined
    // We'll keep a simple items list for now
}

void MainMenuScreen::OnExit() {

}

bool MainMenuScreen::IsModal() const {
    return true;
}

void MainMenuScreen::Update(float dt, InputContext& input) {
    blinkTimer += dt;
    // use semantic navigation
    if (input.IsActionPressed(InputAction::NavigateUp) || input.IsActionPressed(InputAction::DialogueChoicePrev) || input.IsActionPressed(InputAction::PuzzleUp)) {
        selected = std::max(0, selected - 1);
    }
    if (input.IsActionPressed(InputAction::NavigateDown) || input.IsActionPressed(InputAction::DialogueChoiceNext) || input.IsActionPressed(InputAction::PuzzleDown)) {
        selected = std::min<int>(items.size()-1, selected + 1);
    }

    if (input.IsActionPressed(InputAction::Confirm) || input.IsActionPressed(InputAction::DialogueAdvance)) {
        const std::string& choice = items[selected];
        if (choice == "Quit") {
            TransitionManager::Instance().PlayStinger("ui_stinger_quit", 1.0f);
            TransitionManager::Instance().StartAudioFade("music_menu", 0.0f, 1.2f);
            TransitionManager::Instance().StartFade(1.2f, false, [this]() {
                if (onQuit) onQuit();
            });
        } else if (choice == "New Game") {
            TransitionManager::Instance().PlayStinger("ui_stinger_select", 1.0f);
            TransitionManager::Instance().StartAudioFade("music_menu", 0.0f, 1.0f);
            TransitionManager::Instance().StartFade(1.0f, false, [this]() {
                if (onNewGame) onNewGame();
            });
        } else if (choice == "Continue") {
            TransitionManager::Instance().PlayStinger("ui_stinger_select", 1.0f);
            TransitionManager::Instance().StartAudioFade("music_menu", 0.0f, 0.8f);
            TransitionManager::Instance().StartFade(0.8f, false, [this]() {
                if (onContinue) onContinue();
            });
        } else if (choice == "Load Game") {
            if (onLoad) onLoad();
        }
    }
}

void MainMenuScreen::Render(TextRenderer* hud, int screenWidth, int screenHeight) {
    if (!hud) return;

    const float baseY = static_cast<float>(screenHeight) * 0.4f;
    const float centerX = static_cast<float>(screenWidth) * 0.5f;
    const float spacing = 48.0f;

    for (size_t i = 0; i < items.size(); ++i) {
        glm::vec3 color(0.8f, 0.8f, 0.8f);
        if (static_cast<int>(i) == selected) {
            float t = fmod(blinkTimer, 1.0f);
            float pulse = 0.8f + 0.2f * t;
            color = glm::vec3(pulse, pulse, pulse);
        }
        hud->RenderText(items[i], centerX - 100.0f, baseY + static_cast<float>(i) * spacing, 1.0f, color, screenWidth, screenHeight, false);
    }
}
