#include "CharacterManager.h"

#include <algorithm>
#include <cstdio>
#include <iostream>

#include <glm/gtc/matrix_transform.hpp>

#include "core/EventBus.h"
#include "narrative/NarrativeEngine.h"
#include "renderer/ProceduralHumanoid.h"
#include "renderer/ProceduralHair.h"
#include "renderer/ProceduralClothing.h"
#include "renderer/FaceDetailGenerator.h"
#include "renderer/Renderer.h"
#include "renderer/Shader.h"
#include "world/DayNightCycle.h"
#include "ai/NavMesh.h"

namespace {

ProceduralHumanoid::BuildParams BuildParamsForCharacter(const std::string& name) {
    if (name == "Boyd") {
        return ProceduralHumanoid::BuildParams{
            1.85f,
            0.50f,
            0.38f,
            1.02f,
            1.04f,
            1.25f,
            false,
            glm::vec3(0.28f, 0.19f, 0.14f),
        };
    }
    if (name == "Jade") {
        return ProceduralHumanoid::BuildParams{
            1.76f,
            0.43f,
            0.34f,
            0.97f,
            1.0f,
            0.88f,
            false,
            glm::vec3(0.52f, 0.37f, 0.25f),
        };
    }
    if (name == "Tabitha") {
        return ProceduralHumanoid::BuildParams{
            1.70f,
            0.40f,
            0.37f,
            0.95f,
            0.97f,
            0.95f,
            true,
            glm::vec3(0.72f, 0.58f, 0.48f),
        };
    }
    if (name == "Victor") {
        return ProceduralHumanoid::BuildParams{
            1.72f,
            0.40f,
            0.33f,
            0.98f,
            0.99f,
            0.72f,
            false,
            glm::vec3(0.75f, 0.67f, 0.60f),
        };
    }
    if (name == "Sara") {
        return ProceduralHumanoid::BuildParams{
            1.65f,
            0.37f,
            0.35f,
            0.93f,
            0.96f,
            0.78f,
            true,
            glm::vec3(0.78f, 0.68f, 0.61f),
        };
    }

    return ProceduralHumanoid::BuildParams{};
}

} // namespace

HairStyle HairStyleForCharacter(const std::string& name) {
    if (name == "Boyd") return HairStyle::CLOSE_CROP;
    if (name == "Jade") return HairStyle::WAVY_MEDIUM;
    if (name == "Tabitha") return HairStyle::PONYTAIL_LOOSE;
    if (name == "Victor") return HairStyle::LONGISH_SWEPT;
    if (name == "Sara") return HairStyle::LONG_STRAIGHT;
    return HairStyle::CLOSE_CROP;
}

ClothingLayer ShirtForCharacter(const std::string& name) {
    if (name == "Boyd") return ClothingLayer::UNIFORM_SHIRT;
    if (name == "Jade") return ClothingLayer::HOODIE;
    if (name == "Tabitha") return ClothingLayer::SHIRT_FLANNEL;
    if (name == "Victor") return ClothingLayer::KNIT_SWEATER;
    if (name == "Sara") return ClothingLayer::SHIRT_SIMPLE;
    return ClothingLayer::SHIRT_SIMPLE;
}

TrouserStyle TrouserForCharacter(const std::string& name) {
    if (name == "Boyd") return TrouserStyle::CARGO_PANTS;
    if (name == "Jade") return TrouserStyle::SLIM_JEANS;
    if (name == "Tabitha") return TrouserStyle::HIKING_PANTS;
    if (name == "Victor") return TrouserStyle::GREY_TROUSERS;
    if (name == "Sara") return TrouserStyle::PLAIN_BLACK;
    return TrouserStyle::PLAIN_BLACK;
}

EyebrowStyle EyebrowForCharacter(const std::string& name) {
    if (name == "Boyd") return EyebrowStyle::HEAVY_FLAT;
    if (name == "Victor") return EyebrowStyle::THIN_LIGHT;
    if (name == "Jade") return EyebrowStyle::MEDIUM_ARCHED;
    return EyebrowStyle::MEDIUM;
}

LipStyle LipForCharacter(const std::string& name) {
    if (name == "Boyd") return LipStyle::FULL;
    if (name == "Victor") return LipStyle::THIN;
    if (name == "Sara") return LipStyle::NARROW;
    return LipStyle::MEDIUM;
}

NoseStyle NoseForCharacter(const std::string& name) {
    if (name == "Boyd") return NoseStyle::BROAD_FLAT;
    if (name == "Victor") return NoseStyle::NARROW_LONG;
    if (name == "Tabitha" || name == "Sara") return NoseStyle::SMALL_UPTURNED;
    return NoseStyle::MEDIUM_STRAIGHT;
}

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
    EnsureProceduralCharacter(c.get());
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
    (void)renderer;

    if (!camera) {
        return;
    }

    EnsureProceduralShader();
    if (!proceduralShaderReady || !proceduralShader) {
        return;
    }

    const glm::mat4 view = camera->GetViewMatrix();
    const glm::mat4 projection = camera->GetProjectionMatrix(renderer.GetAspectRatio());

    proceduralShader->Bind();
    proceduralShader->SetMat4("view", view);
    proceduralShader->SetMat4("projection", projection);
    proceduralShader->SetFloat("roughness", 0.68f);
    proceduralShader->SetFloat("metalness", 0.02f);
    proceduralShader->SetVec3("emission", glm::vec3(0.0f));

    for (auto& pair : characters) {
        Character* c = pair.second.get();
        EnsureProceduralCharacter(c);
        ProceduralHumanoid* humanoid = c->GetProceduralHumanoid();
        if (!humanoid) {
            continue;
        }

        glm::mat4 root = glm::translate(glm::mat4(1.0f), c->position);
        root = glm::rotate(root, c->facingAngle, glm::vec3(0.0f, 1.0f, 0.0f));
        humanoid->UpdateJointTransform(JointId::ROOT, root);

        proceduralShader->SetVec3("albedoColor", humanoid->GetSkinColor());
        humanoid->Draw(*proceduralShader, {});

        if (c->GetProceduralClothing()) {
            c->GetProceduralClothing()->Draw(*proceduralShader, root);
        }
        if (c->GetProceduralHair()) {
            c->GetProceduralHair()->Draw(*proceduralShader, root);
        }
        if (c->GetFaceDetails()) {
            c->GetFaceDetails()->Draw(*proceduralShader, root);
        }
    }

    proceduralShader->Unbind();
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

void CharacterManager::EnsureProceduralCharacter(Character* c) {
    if (!c || c->GetProceduralHumanoid()) {
        return;
    }

    const std::string name = c->GetCharacterName();
    c->SetProceduralHumanoid(new ProceduralHumanoid(BuildParamsForCharacter(name)));

    auto* hair = new ProceduralHair(HairStyleForCharacter(name), glm::vec3(0.12f, 0.09f, 0.08f), glm::vec3(0.3f, 0.2f, 0.16f), name == "Sara" ? 0.42f : 0.16f);
    hair->GenerateStrands(c->GetProceduralHumanoid());
    c->SetProceduralHair(hair);

    auto* clothing = new ProceduralClothing(ShirtForCharacter(name), TrouserForCharacter(name));
    clothing->Build(c->GetProceduralHumanoid());
    c->SetProceduralClothing(clothing);

    auto* face = new FaceDetailGenerator();
    face->Build(c->GetProceduralHumanoid(), EyebrowForCharacter(name), LipForCharacter(name), NoseForCharacter(name));
    c->SetFaceDetails(face);
}

void CharacterManager::EnsureProceduralShader() {
    if (proceduralShaderReady) {
        return;
    }

    if (!proceduralShader) {
        proceduralShader = std::make_unique<Shader>("ProceduralHumanoid");
    }

    try {
        proceduralShader->Load("assets/shaders/procedural_humanoid.vert", "assets/shaders/procedural_humanoid.frag");
        proceduralShaderReady = true;
    } catch (const std::exception& e) {
        proceduralShaderReady = false;
        std::cerr << "Procedural humanoid shader load failed: " << e.what() << '\n';
    }
}

nlohmann::json CharacterManager::Serialize() const {
    return nlohmann::json(); // SaveSystem handles character serialization specifically
}

void CharacterManager::Deserialize(const nlohmann::json& j) {
    // Handled by SaveSystem
}
