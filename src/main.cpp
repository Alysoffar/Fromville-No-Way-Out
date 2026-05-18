#include "engine/core/Engine.h"
#include "game/ui/MenuSystem.h"

#include <filesystem>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

int main(int argc, char** argv) {
    try {
        if (argc > 0 && argv && argv[0]) {
            const std::filesystem::path exePath = std::filesystem::absolute(argv[0]);
            const std::filesystem::path exeDir = exePath.parent_path();
            if (!exeDir.empty()) {
                std::filesystem::current_path(exeDir);
            }
        }
    } catch (...) {
        // If working dir adjustment fails, keep default behavior.
    }

    Engine engine;
    if (!engine.Initialize(1280, 720, "Fromville No Way Out")) {
        return 1;
    }

    MenuSystem menuSystem;
    if (!menuSystem.Initialize(engine, 1280, 720)) {
        return 1;
    }

    while (!engine.ShouldClose()) {
        engine.PollInput();
        const float dt = engine.Tick();
        
        menuSystem.Update(dt, engine);

        engine.BeginFrame();
        menuSystem.Render(engine);
        engine.EndFrame();
    }

    menuSystem.Shutdown();
    engine.Shutdown();
    return 0;
}

