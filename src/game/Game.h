#pragma once

#include <memory>

#include <glm/glm.hpp>

class Camera;
class Engine;

#include "engine/renderer/Camera.h"
#include "game/world/World.h"
#include "engine/renderer/TerrainRenderer.h"
#include "engine/renderer/GrassRenderer.h"
#include "engine/renderer/TreeRenderer.h"
#include "engine/renderer/SkydomeRenderer.h"
#include "game/world/DayNightCycle.h"
#include "engine/renderer/AnimatedMesh.h"
#include "engine/renderer/Shader.h"
#include "game/PlayerController.h"
#include "engine/renderer/Animation.h"
#include "engine/renderer/Animator.h"
#include "engine/renderer/TextRenderer.h"

class Game {
public:
    Game();

    bool Initialize(Engine& engine);
    void Update(float dt, Engine& engine);
    void Render(Engine& engine) const;
    void Shutdown();

private:
    void UpdateHudTitle(Engine& engine) const;
    void RenderHud(const Engine& engine) const;
    std::unique_ptr<Camera> camera;
    std::unique_ptr<World> world;

    mutable GroundRenderer   groundRenderer;
    mutable GrassRenderer    grassRenderer;
    mutable TreeRenderer     treeRenderer;
    mutable SkydomeRenderer  skydomeRenderer;
    DayNightCycle            dayNightCycle;

    glm::vec3 spawnPosition = glm::vec3(0.0f, 1.8f, 3.5f);
    bool cursorLocked = true;
    
    std::unique_ptr<AnimatedMesh> characterMesh;
    std::unique_ptr<Shader> animatedShader;
    std::unique_ptr<Animation> walkingAnimation;
    std::unique_ptr<Animator> animator;
    PlayerController m_Player;

    mutable TextRenderer textRenderer;
};