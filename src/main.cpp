#include "engine/core/Engine.h"
#include "game/Game.h"

int main() {
    Engine engine;
    if (!engine.Initialize(1280, 720, "Fromville: Engine/Game Split")) {
        return 1;
    }

    Game game;
    if (!game.Initialize(engine)) {
        return 1;
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
