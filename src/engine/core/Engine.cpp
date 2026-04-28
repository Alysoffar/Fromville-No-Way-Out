#include "engine/core/Engine.h"

#include <iostream>

#include <glad/glad.h>

bool Engine::Initialize(int width, int height, const std::string& title) {
    window = std::make_unique<Window>(width, height, title);
    if (!window || !window->Init()) {
        std::cerr << "Failed to create window.\n";
        return false;
    }

    timer = std::make_unique<Timer>();
    input = std::make_unique<InputManager>();
    input->SetWindow(window->GetHandle());
    input->SetCursorLocked(true);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glClearColor(0.08f, 0.09f, 0.12f, 1.0f);

    return true;
}

void Engine::Shutdown() {
    input.reset();
    timer.reset();
    window.reset();
}

void Engine::PollInput() {
    if (input) {
        input->Poll();
    }
}

float Engine::Tick() {
    if (!timer) {
        return 0.0f;
    }

    timer->Tick();
    return timer->GetDeltaTime();
}

void Engine::BeginFrame() const {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Engine::EndFrame() {
    if (window) {
        window->SwapBuffers();
    }
}

bool Engine::ShouldClose() const {
    return !window || window->ShouldClose();
}

float Engine::GetAspectRatio() const {
    return window ? window->GetAspectRatio() : 1.0f;
}

Window& Engine::GetWindow() {
    return *window;
}

const Window& Engine::GetWindow() const {
    return *window;
}

InputManager& Engine::GetInput() {
    return *input;
}

const InputManager& Engine::GetInput() const {
    return *input;
}