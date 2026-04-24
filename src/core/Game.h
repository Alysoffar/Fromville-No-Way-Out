#pragma once

#include <memory>
#include <vector>

#include "core/Window.h"
#include "core/Timer.h"
#include "core/InputManager.h"
#include "core/EventBus.h"
#include "audio/AudioEngine.h"
#include "renderer/Renderer.h"
#include "renderer/PostProcess.h"
#include "renderer/ParticleSystem.h"
#include "world/DayNightCycle.h"
#include "world/World.h"
#include "ai/NavMesh.h"
#include "world/TalismanSystem.h"
#include "ai/WhistleSystem.h"
#include "world/GlyphPuzzle.h"
#include "world/TunnelNetwork.h"
#include "characters/CharacterManager.h"
#include "ai/CreatureAI.h"
#include "ai/NPC.h"
#include "narrative/NarrativeEngine.h"
#include "narrative/DialogueSystem.h"
#include "narrative/QuestLog.h"
#include "core/SaveSystem.h"
#include "ui/HUD.h"

class Game {
public:
    Game();
    ~Game();
    bool Init();
    void Run();
    void Shutdown();

private:
    std::unique_ptr<Window>          window;
    std::unique_ptr<Timer>           timer;
    std::unique_ptr<InputManager>    input;
    EventBus*                        eventBus;       
    AudioEngine*                     audio;

    std::unique_ptr<Renderer>        renderer;
    std::unique_ptr<PostProcess>     postProcess;
    std::unique_ptr<ParticleSystem>  particles;

    std::unique_ptr<DayNightCycle>   dayNight;
    std::unique_ptr<World>           world;
    std::unique_ptr<NavMesh>         navMesh;
    std::unique_ptr<TalismanSystem>  talismanSystem;
    std::unique_ptr<WhistleSystem>   whistleSystem;
    std::unique_ptr<GlyphPuzzle>     glyphPuzzle;
    std::unique_ptr<GlyphRegistry>   glyphRegistry;
    std::unique_ptr<TunnelNetwork>   tunnelNetwork;

    std::unique_ptr<CharacterManager> characterManager;
    std::unique_ptr<CreatureAI>       creatureAI;
    std::vector<std::unique_ptr<NPC>> npcs;

    std::unique_ptr<NarrativeEngine>  narrativeEngine;
    std::unique_ptr<DialogueSystem>   dialogueSystem;
    std::unique_ptr<QuestLog>         questLog;
    SaveSystem*                       saveSystem;

    std::unique_ptr<HUD>              hud;

    bool isPaused = false;
    bool isDead = false;
    std::string deadCharacterName;

    void Update(float dt);
    void Render();
    void HandleGlobalInput();
    void TriggerDeathScreen(const std::string& characterName);
    HUDState BuildHUDState() const;
};
