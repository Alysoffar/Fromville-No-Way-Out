#include "CharacterManager.h"

#include <algorithm>
#include <cstdio>
#include <iostream>

#include "core/EventBus.h"
#include "narrative/NarrativeEngine.h"
#include "world/DayNightCycle.h"
#include "ai/NavMesh.h"

CharacterManager::CharacterManager(EventBus* bus, NarrativeEngine* narrative)
    : bus(bus), narrative(narrative) {
    if (bus) {
        bus->Subscribe(GameEvent::CHARACTER_SWITCH_REQUESTED, [this](const EventData& e) {
            this->OnCharacterSwitchRequested(e);
        });
    }
}

CharacterManager::~CharacterManager() {}

void CharacterManager::Init(World* w, NavMesh* nm, AudioEngine* a) {
    world = w;
    navMesh = nm;
    audio = a;
    camera = std::make_unique<Camera>();
}

void CharacterManager::RegisterCharacter(std::unique_ptr<Character> c) {
    if (!c) return;
    std::string n = c->name;
    characters[n] = std::move(c);
}

void CharacterManager::SetActiveCharacter(const std::string& name) {
    auto it = characters.find(name);
    if (it == characters.end()) return;

    if (activeCharacter) {
        activeCharacter->OnBecomeInactive();
        activeCharacter->isActivePlayer = false;
    }

    activeCharacter = it->second.get();
    activeCharacter->isActivePlayer = true;
    activeCharacter->OnBecomeActive();

    playerController = std::make_unique<PlayerController>(activeCharacter, camera.get());
    if (camera) {
        camera->Reset(activeCharacter->position);
    }
}

Character* CharacterManager::GetCharacter(const std::string& name) {
    auto it = characters.find(name);
    if (it != characters.end()) return it->second.get();
    return nullptr;
}

std::vector<Character*> CharacterManager::GetAll() {
    std::vector<Character*> res;
    for (auto& pair : characters) {
        res.push_back(pair.second.get());
    }
    return res;
}

void CharacterManager::UpdateActive(float dt, InputManager& input, Camera& cam) {
    if (activeCharacter) {
        if (playerController) {
            playerController->Update(dt, input);
        }
        activeCharacter->Update(dt);
        activeCharacter->HandleInput(input, cam);
    }
}

void CharacterManager::UpdateOffscreen(float dt) {
    for (auto& pair : characters) {
        Character* c = pair.second.get();
        if (c != activeCharacter && c->IsAlive()) {
            SimulateOffscreenCharacter(c, dt);
        }
    }
}

void CharacterManager::DrawAll(Renderer& renderer) {
    // Left unimplemented for now as per spec
}

bool CharacterManager::IsInsideSafeBuilding(Character* c) const {
    // Placeholder implementation - assumes inside if close to origin
    if (!c) return false;
    return glm::length(c->position) < 5.0f; 
}

bool CharacterManager::CanSwitch() const {
    if (DayNightCycle::Get().IsDaytime()) return true;
    return IsInsideSafeBuilding(activeCharacter);
}

void CharacterManager::RequestSwitch(const std::string& toName) {
    if (!CanSwitch()) {
        std::printf("Cannot switch now - not safe\n");
        return;
    }

    auto it = characters.find(toName);
    if (it == characters.end()) return;

    SetActiveCharacter(toName);

    if (bus) {
        std::unordered_map<std::string, std::variant<int, float, bool, std::string, glm::vec3>> payload;
        payload["to"] = toName;
        bus->Fire(GameEvent::CHARACTER_SWITCHED, payload);
    }
}

void CharacterManager::OnCharacterSwitchRequested(const EventData& e) {
    auto it = e.payload.find("to");
    if (it != e.payload.end() && std::holds_alternative<std::string>(it->second)) {
        RequestSwitch(std::get<std::string>(it->second));
    }
}

void CharacterManager::SimulateOffscreenCharacter(Character* c, float dt) {
    if (!DayNightCycle::Get().IsDaytime() && !IsInsideSafeBuilding(c)) {
        // Simplified logic: move towards safe building (origin)
        if (navMesh) {
            glm::vec3 dir = glm::vec3(0.0f) - c->position;
            float dist = glm::length(dir);
            if (dist > 0.1f) {
                dir /= dist;
                c->position += dir * c->stats.moveSpeed * dt;
            }
        }
        // Simplified damage logic
        if (glm::length(c->position) > 10.0f) {
            c->stats.health = std::max(1.0f, c->stats.health - 5.0f * dt);
        }
    }
}

nlohmann::json CharacterManager::Serialize() const {
    return nlohmann::json(); // SaveSystem handles character serialization specifically
}

void CharacterManager::Deserialize(const nlohmann::json& j) {
    // Handled by SaveSystem
}
