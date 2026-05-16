#pragma once

#include "game/runtime/Timers.h"

class WorldManager {
public:
    void UpdateWorldSimulation(float dt);

private:
    DurationTimer heavySystemTickGate;
};
