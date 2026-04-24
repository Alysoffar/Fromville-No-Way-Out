#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

#include "characters/Character.h"
#include "characters/PlayerController.h"
#include "renderer/Camera.h"

class AudioEngine;
class EventBus;
class InputManager;
class NarrativeEngine;
class NavMesh;
class Renderer;
class World;
class Shader;

struct EventData;

class CharacterManager {
public:
    CharacterManager(EventBus* bus, NarrativeEngine* narrative);
    ~CharacterManager();

    void Init(World* world, NavMesh* navMesh, AudioEngine* audio);
    void RegisterCharacter(std::unique_ptr<Character> c);
    void SetActiveCharacter(const std::string& name);
    
    Character* GetActive() const { return activeCharacter; }
    Character* GetCharacter(const std::string& name);
    std::vector<Character*> GetAll();
    Camera* GetCamera() const { return camera.get(); }

    void UpdateActive(float dt, InputManager& input, Camera& camera);
    void UpdateOffscreen(float dt);
    void DrawAll(Renderer& renderer);
    
    bool CanSwitch() const;   
    void RequestSwitch(const std::string& toName);

    nlohmann::json Serialize() const;
    void Deserialize(const nlohmann::json& j);

private:
    std::unordered_map<std::string, std::unique_ptr<Character>> characters;
    Character* activeCharacter = nullptr;
    EventBus* bus = nullptr;
    NarrativeEngine* narrative = nullptr;
    World* world = nullptr;
    NavMesh* navMesh = nullptr;
    AudioEngine* audio = nullptr;

    std::unique_ptr<PlayerController> playerController;
    std::unique_ptr<Camera> camera;
    std::unique_ptr<Shader> proceduralShader;
    bool proceduralShaderReady = false;

    void OnCharacterSwitchRequested(const EventData& e);
    void SimulateOffscreenCharacter(Character* c, float dt);
    bool IsInsideSafeBuilding(Character* c) const;
    void EnsureProceduralCharacter(Character* c);
    void EnsureProceduralShader();
};
