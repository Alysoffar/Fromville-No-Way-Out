#include "game/world/WorldManager.h"

#include "game/atmosphere/AtmosphereManager.h"
#include "game/memory/MemoryReplaySystem.h"
#include "game/moral/MoralCorruptionSystem.h"
#include "game/story/StoryManager.h"
#include "game/symbols/SymbolSystem.h"
#include "game/tunnels/TunnelMappingSystem.h"

void WorldManager::UpdateWorldSimulation(float dt) {
    StoryManager::Instance().Update(dt);
    AtmosphereManager::Instance().Update(dt);
    SymbolSystem::Instance().Update(dt);
    SymbolSystem::Instance().UpdateSymbolSight(dt);

    // Heavy narrative systems can be throttled slightly without visible gameplay loss.
    heavySystemTickGate.Tick(dt);
    if (!heavySystemTickGate.IsActive()) {
        TunnelMappingSystem::Instance().Update(dt);
        MemoryReplaySystem::Instance().Update(dt);
        MoralCorruptionSystem::Instance().Update(dt);
        heavySystemTickGate.Start(0.05f);
    }
}
