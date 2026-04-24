#include "Game.h"

#include <iostream>

#include <GLFW/glfw3.h>

#include "characters/Boyd.h"
#include "characters/Jade.h"
#include "characters/Tabitha.h"
#include "characters/Victor.h"
#include "characters/Sara.h"

Game::Game() {}

Game::~Game() {
    Shutdown();
}

bool Game::Init() {
    window = std::make_unique<Window>(1280, 720, "Fromville: No Way Out");
    if (!window->Init()) return false;

    timer = std::make_unique<Timer>();
    
    input = std::make_unique<InputManager>();
    // Assuming Window has GetHandle() or similar
    input->SetWindow(window->GetHandle());

    eventBus = &EventBus::Get();

    audio = &AudioEngine::Get();
    audio->Init();

    renderer = std::make_unique<Renderer>(1280, 720);
    renderer->Init();

    postProcess = std::make_unique<PostProcess>(1280, 720);
    postProcess->Init();

    particles = std::make_unique<ParticleSystem>();

    dayNight = std::make_unique<DayNightCycle>();

    glyphRegistry = std::make_unique<GlyphRegistry>();

    navMesh = std::make_unique<NavMesh>();
    navMesh->LoadFromJSON("assets/levels/navmesh.json");

    world = std::make_unique<World>();
    world->Load("assets/levels/fromville.json");

    tunnelNetwork = std::make_unique<TunnelNetwork>();
    // tunnelNetwork->Load(...);

    talismanSystem = std::make_unique<TalismanSystem>();
    nlohmann::json dummyLevel;
    talismanSystem->LoadNodesFromLevel(dummyLevel);

    questLog = std::make_unique<QuestLog>();

    narrativeEngine = std::make_unique<NarrativeEngine>(questLog.get(), eventBus);

    dialogueSystem = std::make_unique<DialogueSystem>(glyphRegistry.get(), narrativeEngine.get());
    dialogueSystem->LoadDialogues("assets/levels/dialogues.json");

    glyphPuzzle = std::make_unique<GlyphPuzzle>(glyphRegistry.get());
    // glyphPuzzle->LoadFromLevel(...);

    whistleSystem = std::make_unique<WhistleSystem>(talismanSystem.get(), audio);

    auto boyd = std::make_unique<Boyd>();
    auto jade = std::make_unique<Jade>();
    auto tabitha = std::make_unique<Tabitha>();
    auto victor = std::make_unique<Victor>();
    auto sara = std::make_unique<Sara>();

    characterManager = std::make_unique<CharacterManager>(eventBus, narrativeEngine.get());
    characterManager->RegisterCharacter(std::move(boyd));
    characterManager->RegisterCharacter(std::move(jade));
    characterManager->RegisterCharacter(std::move(tabitha));
    characterManager->RegisterCharacter(std::move(victor));
    characterManager->RegisterCharacter(std::move(sara));

    characterManager->Init(world.get(), navMesh.get(), audio);

    creatureAI = std::make_unique<CreatureAI>(navMesh.get(), dayNight.get());
    // Spawn 6 initial creatures with patrol paths from level JSON
    // creatureAI->SpawnCreature(...);

    std::vector<Character*> allCharsInit = characterManager->GetAll();
    narrativeEngine->Init(allCharsInit);
    narrativeEngine->LoadQuestsFromJSON("assets/levels/quests.json");

    hud = std::make_unique<HUD>();
    hud->Init(renderer.get());

    input->SetCursorLocked(true);

    saveSystem = &SaveSystem::Get();
    std::vector<NPC*> npcRawPtrs;
    for (auto& npc : npcs) npcRawPtrs.push_back(npc.get());
    
    if (saveSystem->AutoSaveExists()) {
        saveSystem->LoadGame("autosave", *characterManager, *narrativeEngine, *world, *dayNight, npcRawPtrs);
    }

    if (!characterManager->GetActive()) {
        characterManager->SetActiveCharacter("Boyd");
    }

    eventBus->Subscribe(GameEvent::PLAYER_DIED, [this](const EventData& e) {
        auto it = e.payload.find("character");
        if (it != e.payload.end()) {
            TriggerDeathScreen(std::get<std::string>(it->second));
        }
    });

    return true;
}

void Game::Run() {
    while (!window->ShouldClose()) {
        timer->Tick();
        float dt = timer->GetDeltaTime();

        input->Poll();
        HandleGlobalInput();

        if (!isPaused) {
            Update(dt);
        }
        Render();
        window->SwapBuffers();
    }
}

void Game::Shutdown() {
    if (audio) {
        audio->Shutdown();
    }
}

void Game::Update(float dt) {
    EventBus::Get().ProcessQueued();
    
    dayNight->Update(dt);
    
    Character* activeChar = characterManager->GetActive();
    Camera* camera = characterManager->GetCamera(); // Accessing through getter or assumes friend class
    
    if (activeChar && camera) {
        audio->Update(activeChar->position, camera->GetForward(), camera->GetUp());
        characterManager->UpdateActive(dt, *input, *camera);
    }
    
    characterManager->UpdateOffscreen(dt);
    narrativeEngine->ProcessEvents();
    questLog->Update();
    
    for (auto& npc : npcs) {
        npc->Update(dt);
    }
    
    std::vector<NPC*> npcRawPtrs;
    for (auto& npc : npcs) npcRawPtrs.push_back(npc.get());
    
    std::vector<Character*> allChars = characterManager->GetAll();
    
    creatureAI->Update(dt, allChars, npcRawPtrs);
    talismanSystem->Update(dt);
    
    // Convert creatures to raw pointers
    std::vector<Creature*> creatureRawPtrs;
    for (auto& c : creatureAI->GetCreaturesMutable()) {
        creatureRawPtrs.push_back(&c);
    }
    whistleSystem->Update(dt, creatureRawPtrs, npcRawPtrs, allChars);
    
    glyphPuzzle->Update(dt, *input);
    particles->Update(dt);
    
    hud->Update(dt, BuildHUDState());
}

void Game::Render() {
    Camera* cam = characterManager->GetCamera();
    if (!cam) return;

    renderer->BeginGeometryPass();
    world->Draw(*renderer, *cam);
    characterManager->DrawAll(*renderer);
    creatureAI->DrawAll(*renderer);
    
    bool symbolSightActive = false;
    if (auto jade = dynamic_cast<Jade*>(characterManager->GetActive())) {
        symbolSightActive = jade->symbolSightActive;
    }
    glyphPuzzle->DrawGlyphsInWorld(*renderer, symbolSightActive);
    renderer->EndGeometryPass();

    std::vector<RenderCommand> emptyCmds;
    renderer->ShadowPass(emptyCmds, dayNight->GetSunDirection());

    renderer->SSAOPass();
    renderer->LightingPass(*dayNight, *cam);

    // Forward pass (particles — alpha blended, rendered after deferred)
    renderer->GetGBuffer()->BlitDepthTo(0, window->GetWidth(), window->GetHeight());
    particles->Draw(*renderer, *cam);

    // Post process
    if (auto active = characterManager->GetActive()) {
        renderer->PostProcessPass(active->GetPostFXState());
    } else {
        PostFXState defaultState;
        renderer->PostProcessPass(defaultState);
    }

    hud->Draw();

    if (isDead) {
        hud->DrawDeathScreen(deadCharacterName);
    }

    if (dialogueSystem->IsActive()) {
        dialogueSystem->Draw(*renderer);
    }
    // glyphPuzzle render overlay already in Draw()
}

void Game::HandleGlobalInput() {
    if (isDead) {
        if (input->IsKeyPressed(GLFW_KEY_R)) {
            std::vector<NPC*> npcRawPtrs;
            for (auto& npc : npcs) npcRawPtrs.push_back(npc.get());
            if (saveSystem->LoadGame("autosave", *characterManager, *narrativeEngine, *world, *dayNight, npcRawPtrs)) {
                isDead = false;
                isPaused = false;
                input->SetCursorLocked(true);
            }
        }
        if (input->IsKeyPressed(GLFW_KEY_ESCAPE)) {
            window->Close();
        }
        return;
    }

    if (input->IsKeyPressed(GLFW_KEY_ESCAPE)) {
        isPaused = !isPaused;
        input->SetCursorLocked(!isPaused);
    }
    
    std::vector<NPC*> npcRawPtrs;
    for (auto& npc : npcs) npcRawPtrs.push_back(npc.get());
    
    if (input->IsKeyPressed(GLFW_KEY_F5)) {
        saveSystem->SaveGame("manual", *characterManager, *narrativeEngine, *world, *dayNight, npcRawPtrs);
    }
    if (input->IsKeyPressed(GLFW_KEY_F9)) {
        saveSystem->LoadGame("autosave", *characterManager, *narrativeEngine, *world, *dayNight, npcRawPtrs);
    }
    if (input->IsKeyPressed(GLFW_KEY_F3)) {
        // Toggle debug renderer (NavMesh wireframe, creature detection cones)
    }
}

void Game::TriggerDeathScreen(const std::string& characterName) {
    isDead = true;
    // isPaused = true;
    deadCharacterName = characterName;
    input->SetCursorLocked(false);
}

HUDState Game::BuildHUDState() const {
    Character* c = characterManager->GetActive();
    HUDState s;
    if (!c) return s;

    s.health = c->stats.health;
    s.maxHealth = c->stats.maxHealth;
    s.stamina = c->stats.stamina;
    s.maxStamina = c->stats.maxStamina;
    s.characterName = c->GetCharacterName();
    
    if (auto boyd = dynamic_cast<Boyd*>(c)) {
        s.curseMeter = boyd->curseMeter;
    }
    if (auto sara = dynamic_cast<Sara*>(c)) {
        s.redemptionScore = sara->redemptionScore;
        s.isGhostStep = sara->ghostStepActive;
    }
    if (auto jade = dynamic_cast<Jade*>(c)) {
        s.symbolSightActive = jade->symbolSightActive;
    }
    if (auto victor = dynamic_cast<Victor*>(c)) {
        s.memoryModeActive = victor->memoryModeActive;
        s.breathHoldRemaining = victor->breathHoldTimer; // Guessed field name, will fix if wrong
    }

    // Set interact/ability hints based on nearest interactable and character type
    s.interactHint = ""; // Needs actual interaction logic
    s.activeAbilityHint = ""; 

    s.puzzleActive = glyphPuzzle->IsPuzzleActive();
    s.dialogueActive = dialogueSystem->IsActive();
    s.showSwitchMenu = input->IsKeyPressed(GLFW_KEY_TAB);

    for (Character* other : characterManager->GetAll()) {
        float hpPercent = (other->stats.health / other->stats.maxHealth) * 100.0f;
        s.characterPortraits.push_back({other->GetCharacterName(), hpPercent});
    }

    return s;
}
