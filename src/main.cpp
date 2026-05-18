#include "engine/core/Engine.h"
#include "game/Game.h"

#include <filesystem>

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
    if (!engine.Initialize(1280, 720, "Fromville: Engine/Game Split")) {
        return 1;
    }

    Game game;
    if (!game.Initialize(engine)) {
        return 1;
    }

    // Command-line debug: --fast-night advances the world clock by 70s once at startup
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--fast-night") {
            game.RequestAdvanceWorldClock(70.0f);
        }
    }

    while (!engine.ShouldClose()) {
        engine.PollInput();
        const float dt = engine.Tick();
        game.Update(dt, engine);
        engine.BeginFrame();
        game.Render(engine);
        engine.EndFrame();
    }

    game.Shutdown();
    engine.Shutdown();
    return 0;
}
