#pragma once

#include <memory>
#include <string>

#include "engine/core/InputManager.h"
#include "engine/core/Timer.h"
#include "engine/core/Window.h"

class Engine {
public:
    Engine() = default;

    bool Initialize(int width, int height, const std::string& title);
    void Shutdown();

    void PollInput();
    float Tick();
    void BeginFrame() const;
    void EndFrame();

    bool ShouldClose() const;
    float GetAspectRatio() const;

    Window& GetWindow();
    const Window& GetWindow() const;
    InputManager& GetInput();
    const InputManager& GetInput() const;

private:
    std::unique_ptr<Window> window;
    std::unique_ptr<Timer> timer;
    std::unique_ptr<InputManager> input;
};