#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "characters/Boyd.h"
#include "characters/Character.h"
#include "characters/Jade.h"
#include "characters/Sara.h"
#include "characters/Tabitha.h"
#include "characters/Victor.h"
#include "core/Timer.h"
#include "core/Window.h"
#include "renderer/Camera.h"
#include "renderer/FaceDetailGenerator.h"
#include "renderer/PostFXState.h"
#include "renderer/ProceduralClothing.h"
#include "renderer/ProceduralHair.h"
#include "renderer/ProceduralHumanoid.h"
#include "renderer/Renderer.h"
#include "renderer/Shader.h"
#include "world/DayNightCycle.h"
#include "world/WorldBuilder.h"

int RunNarrativeTests();

namespace {

ProceduralHumanoid::BuildParams BuildParamsForCharacter(const std::string& name) {
    if (name == "Boyd") return {1.90f, 0.60f, 0.43f, 1.10f, 1.02f, 1.42f, false, glm::vec3(0.35f, 0.26f, 0.19f)};
    if (name == "Jade") return {1.76f, 0.43f, 0.34f, 0.97f, 1.00f, 0.88f, false, glm::vec3(0.52f, 0.37f, 0.25f)};
    if (name == "Tabitha") return {1.70f, 0.40f, 0.37f, 0.95f, 0.97f, 0.95f, true, glm::vec3(0.72f, 0.58f, 0.48f)};
    if (name == "Victor") return {1.72f, 0.40f, 0.33f, 0.98f, 0.99f, 0.72f, false, glm::vec3(0.75f, 0.67f, 0.60f)};
    if (name == "Sara") return {1.65f, 0.37f, 0.35f, 0.93f, 0.96f, 0.78f, true, glm::vec3(0.78f, 0.68f, 0.61f)};
    return {};
}

HairStyle HairStyleForCharacter(const std::string& name) {
    if (name == "Boyd") return HairStyle::CREATURE_MATTED;
    if (name == "Jade") return HairStyle::WAVY_MEDIUM;
    if (name == "Tabitha") return HairStyle::PONYTAIL_LOOSE;
    if (name == "Victor") return HairStyle::LONGISH_SWEPT;
    if (name == "Sara") return HairStyle::LONG_STRAIGHT;
    return HairStyle::CLOSE_CROP;
}

ClothingLayer ShirtForCharacter(const std::string& name) {
    if (name == "Boyd") return ClothingLayer::CREATURE_JACKET;
    if (name == "Jade") return ClothingLayer::HOODIE;
    if (name == "Tabitha") return ClothingLayer::SHIRT_FLANNEL;
    if (name == "Victor") return ClothingLayer::KNIT_SWEATER;
    return ClothingLayer::SHIRT_SIMPLE;
}

TrouserStyle TrouserForCharacter(const std::string& name) {
    if (name == "Boyd") return TrouserStyle::CARGO_PANTS;
    if (name == "Jade") return TrouserStyle::SLIM_JEANS;
    if (name == "Tabitha") return TrouserStyle::HIKING_PANTS;
    if (name == "Victor") return TrouserStyle::GREY_TROUSERS;
    return TrouserStyle::PLAIN_BLACK;
}

EyebrowStyle BrowForCharacter(const std::string& name) {
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

void AttachProceduralAppearance(Character& character) {
    const std::string name = character.GetCharacterName();
    character.SetProceduralHumanoid(new ProceduralHumanoid(BuildParamsForCharacter(name)));

    glm::vec3 hairBase(0.12f, 0.09f, 0.08f);
    glm::vec3 hairHighlight(0.30f, 0.20f, 0.16f);
    if (name == "Tabitha") {
        hairBase = glm::vec3(0.42f, 0.18f, 0.08f);
        hairHighlight = glm::vec3(0.65f, 0.33f, 0.16f);
    } else if (name == "Boyd") {
        hairBase = glm::vec3(0.05f, 0.04f, 0.04f);
        hairHighlight = glm::vec3(0.16f, 0.12f, 0.10f);
    } else if (name == "Sara") {
        hairBase = glm::vec3(0.08f, 0.05f, 0.05f);
        hairHighlight = glm::vec3(0.18f, 0.12f, 0.10f);
    } else if (name == "Victor") {
        hairBase = glm::vec3(0.45f, 0.42f, 0.37f);
        hairHighlight = glm::vec3(0.72f, 0.70f, 0.66f);
    }

    auto* hair = new ProceduralHair(HairStyleForCharacter(name), hairBase, hairHighlight, name == "Sara" ? 0.42f : 0.18f);
    hair->GenerateStrands(character.GetProceduralHumanoid());
    character.SetProceduralHair(hair);

    auto* clothing = new ProceduralClothing(ShirtForCharacter(name), TrouserForCharacter(name));
    clothing->Build(character.GetProceduralHumanoid());
    character.SetProceduralClothing(clothing);

    auto* face = new FaceDetailGenerator();
    face->Build(character.GetProceduralHumanoid(), BrowForCharacter(name), LipForCharacter(name), NoseForCharacter(name));
    character.SetFaceDetails(face);
}

float RootHeightBias(const Character& character) {
    if (const ProceduralHumanoid* human = character.GetProceduralHumanoid()) {
        return human->GetBuildParams().heightMeters * 0.028f;
    }
    return 0.05f;
}

float LerpAngle(float current, float target, float factor) {
    float delta = std::fmod(target - current + glm::pi<float>(), glm::two_pi<float>()) - glm::pi<float>();
    if (delta < -glm::pi<float>()) {
        delta += glm::two_pi<float>();
    }
    return current + delta * factor;
}

bool ConsumePress(GLFWwindow* handle, int key, bool& previousDown) {
    const bool down = glfwGetKey(handle, key) == GLFW_PRESS;
    const bool pressed = down && !previousDown;
    previousDown = down;
    return pressed;
}

void UpdateCameraOrbit(Camera& camera,
                       GLFWwindow* handle,
                       float dt,
                       bool& firstMouse,
                       double& lastMouseX,
                       double& lastMouseY) {
    if (glfwGetMouseButton(handle, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        double mouseX = 0.0;
        double mouseY = 0.0;
        glfwGetCursorPos(handle, &mouseX, &mouseY);

        if (firstMouse) {
            lastMouseX = mouseX;
            lastMouseY = mouseY;
            firstMouse = false;
        }

        const float mouseSensitivity = 0.085f;
        camera.ProcessMouseMovement(static_cast<float>(mouseX - lastMouseX) * mouseSensitivity,
                                    static_cast<float>(lastMouseY - mouseY) * mouseSensitivity);
        lastMouseX = mouseX;
        lastMouseY = mouseY;
    } else {
        firstMouse = true;
    }

    const float keyTurn = 92.0f * dt;
    if (glfwGetKey(handle, GLFW_KEY_LEFT) == GLFW_PRESS) camera.ProcessMouseMovement(-keyTurn, 0.0f);
    if (glfwGetKey(handle, GLFW_KEY_RIGHT) == GLFW_PRESS) camera.ProcessMouseMovement(keyTurn, 0.0f);
    if (glfwGetKey(handle, GLFW_KEY_UP) == GLFW_PRESS) camera.ProcessMouseMovement(0.0f, keyTurn);
    if (glfwGetKey(handle, GLFW_KEY_DOWN) == GLFW_PRESS) camera.ProcessMouseMovement(0.0f, -keyTurn);
}

void UpdateActiveCharacter(Character& activeCharacter, const Camera& camera, GLFWwindow* handle, float dt) {
    glm::vec3 cameraForward = camera.GetForward();
    cameraForward.y = 0.0f;
    if (glm::length(cameraForward) < 1e-4f) {
        cameraForward = glm::vec3(0.0f, 0.0f, -1.0f);
    } else {
        cameraForward = glm::normalize(cameraForward);
    }
    const glm::vec3 cameraRight = glm::normalize(glm::cross(cameraForward, glm::vec3(0.0f, 1.0f, 0.0f)));

    glm::vec3 moveDirection(0.0f);
    if (glfwGetKey(handle, GLFW_KEY_W) == GLFW_PRESS) moveDirection += cameraForward;
    if (glfwGetKey(handle, GLFW_KEY_S) == GLFW_PRESS) moveDirection -= cameraForward;
    if (glfwGetKey(handle, GLFW_KEY_A) == GLFW_PRESS) moveDirection -= cameraRight;
    if (glfwGetKey(handle, GLFW_KEY_D) == GLFW_PRESS) moveDirection += cameraRight;

    const bool crouching = glfwGetKey(handle, GLFW_KEY_C) == GLFW_PRESS;
    bool sprinting = glfwGetKey(handle, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS && !crouching;
    if (activeCharacter.stats.stamina <= 1.0f) {
        sprinting = false;
    }

    const float regenScale = crouching ? 0.65f : 1.0f;
    if (glm::length(moveDirection) > 1e-4f) {
        moveDirection = glm::normalize(moveDirection);

        float speed = activeCharacter.stats.moveSpeed;
        if (crouching) {
            speed = activeCharacter.stats.crouchSpeed;
            activeCharacter.state = CharacterState::CROUCHING;
            activeCharacter.SetNoiseLevelThisFrame(1.0f);
        } else if (sprinting) {
            speed = activeCharacter.stats.runSpeed;
            activeCharacter.state = CharacterState::RUNNING;
            activeCharacter.SetNoiseLevelThisFrame(8.0f);
            activeCharacter.UseStamina(22.0f * dt);
        } else {
            activeCharacter.state = CharacterState::WALKING;
            activeCharacter.SetNoiseLevelThisFrame(3.0f);
            activeCharacter.stats.stamina = std::min(activeCharacter.stats.maxStamina,
                                                     activeCharacter.stats.stamina + activeCharacter.stats.staminaRegenRate * dt * 0.45f);
        }

        activeCharacter.Move(moveDirection, speed, dt);
        const float targetAngle = std::atan2(moveDirection.x, moveDirection.z);
        activeCharacter.facingAngle = LerpAngle(activeCharacter.facingAngle, targetAngle, std::clamp(8.0f * dt, 0.0f, 1.0f));
    } else {
        activeCharacter.state = crouching ? CharacterState::CROUCHING : CharacterState::IDLE;
        activeCharacter.SetNoiseLevelThisFrame(crouching ? 0.5f : 0.0f);
        activeCharacter.stats.stamina = std::min(activeCharacter.stats.maxStamina,
                                                 activeCharacter.stats.stamina + activeCharacter.stats.staminaRegenRate * dt * regenScale);
    }
}

} // namespace

int main() {
    const char* runTestsEnv = std::getenv("FROMVILLE_RUN_TESTS");
    if (runTestsEnv && runTestsEnv[0] != '\0' && runTestsEnv[0] != '0') {
        const int testResult = RunNarrativeTests();
        if (testResult != 0) {
            std::fprintf(stderr, "[main] Narrative tests FAILED - aborting launch\n");
            return testResult;
        }
        std::printf("[main] All narrative tests passed.\n");
    } else {
        std::printf("[main] Skipping headless narrative tests for normal launch.\n");
    }

    Window window(1280, 720, "Fromville: No Way Out - Third Person Showcase");
    if (!window.Init()) {
        std::fprintf(stderr, "Failed to initialize Fromville window\n");
        return 1;
    }

    Renderer renderer(window.GetWidth(), window.GetHeight());
    if (!renderer.Init()) {
        std::fprintf(stderr, "Failed to initialize deferred renderer\n");
        return 1;
    }

    Shader characterShader("ProceduralHumanoid");
    try {
        characterShader.Load("assets/shaders/procedural_humanoid.vert", "assets/shaders/procedural_humanoid.frag");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Failed to load character shader: %s\n", e.what());
        return 1;
    }

    DayNightCycle& dayNight = DayNightCycle::Get();
    dayNight.SetLoopPhase(GameLoopPhase::Explore);
    dayNight.SetCycleDurationSeconds(12.0f * 60.0f);

    WorldBuilder world(nullptr, nullptr);
    world.BuildAll();

    std::vector<std::unique_ptr<Character>> characters;
    characters.emplace_back(std::make_unique<Boyd>());
    characters.emplace_back(std::make_unique<Jade>());
    characters.emplace_back(std::make_unique<Tabitha>());
    characters.emplace_back(std::make_unique<Victor>());
    characters.emplace_back(std::make_unique<Sara>());

    for (auto& character : characters) {
        AttachProceduralAppearance(*character);
    }

    const std::array<glm::vec2, 5> showcaseSpots = {{
        {-18.0f, -12.0f},
        { -2.0f,  10.0f},
        { 18.0f, -14.0f},
        {-34.0f,  18.0f},
        { 20.0f,  22.0f},
    }};

    const std::array<float, 5> facingAngles = {{
        glm::radians(22.0f),
        glm::radians(-18.0f),
        glm::radians(42.0f),
        glm::radians(-32.0f),
        glm::radians(138.0f),
    }};

    for (std::size_t i = 0; i < characters.size(); ++i) {
        characters[i]->position.x = showcaseSpots[i].x;
        characters[i]->position.z = showcaseSpots[i].y;
        characters[i]->position.y = world.GetHeightAt(showcaseSpots[i].x, showcaseSpots[i].y) - RootHeightBias(*characters[i]);
        characters[i]->facingAngle = facingAngles[i];
    }

    int activeCharacterIndex = 0;
    characters[static_cast<std::size_t>(activeCharacterIndex)]->isActivePlayer = true;

    Camera camera;
    camera.yaw = 215.0f;
    camera.pitch = 18.0f;
    camera.farPlane = 900.0f;
    camera.Reset(characters[static_cast<std::size_t>(activeCharacterIndex)]->position);

    std::printf("Showcase controls:\n");
    std::printf("  Move: WASD | Sprint: Left Shift | Crouch: C\n");
    std::printf("  Camera: Hold Right Mouse or use Arrow Keys | Recenter: Space\n");
    std::printf("  Character switch: Tab or 1-5\n");
    std::printf("  Game phases: F1 Explore, F2 Prepare, F3 Survive, F4 Switch Character, F5 Toggle Auto Cycle\n");

    Timer timer;
    int lastWidth = window.GetWidth();
    int lastHeight = window.GetHeight();
    bool firstMouse = true;
    double lastMouseX = 0.0;
    double lastMouseY = 0.0;

    bool tabPrev = false;
    bool spacePrev = false;
    bool f1Prev = false;
    bool f2Prev = false;
    bool f3Prev = false;
    bool f4Prev = false;
    bool f5Prev = false;
    std::array<bool, 5> numberPrev = {{false, false, false, false, false}};
    const std::array<int, 5> numberKeys = {{GLFW_KEY_1, GLFW_KEY_2, GLFW_KEY_3, GLFW_KEY_4, GLFW_KEY_5}};

    while (!window.ShouldClose()) {
        window.PollEvents();

        if (glfwGetKey(window.GetHandle(), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window.GetHandle(), GLFW_TRUE);
        }

        if (window.GetWidth() != lastWidth || window.GetHeight() != lastHeight) {
            lastWidth = window.GetWidth();
            lastHeight = window.GetHeight();
            renderer.Resize(lastWidth, lastHeight);
        }

        timer.Tick();
        const float dt = std::min(timer.GetDeltaTime(), 0.033f);
        const float time = timer.GetTotalTime();

        Character& activeCharacter = *characters[static_cast<std::size_t>(activeCharacterIndex)];
        activeCharacter.isActivePlayer = true;

        if (ConsumePress(window.GetHandle(), GLFW_KEY_TAB, tabPrev)) {
            activeCharacter.isActivePlayer = false;
            activeCharacterIndex = (activeCharacterIndex + 1) % static_cast<int>(characters.size());
            const std::string switchedName = characters[static_cast<std::size_t>(activeCharacterIndex)]->GetCharacterName();
            std::printf("[showcase] Switched to %s\n", switchedName.c_str());
            camera.Reset(characters[static_cast<std::size_t>(activeCharacterIndex)]->position);
        }

        for (std::size_t i = 0; i < numberKeys.size(); ++i) {
            if (ConsumePress(window.GetHandle(), numberKeys[i], numberPrev[i])) {
                characters[static_cast<std::size_t>(activeCharacterIndex)]->isActivePlayer = false;
                activeCharacterIndex = static_cast<int>(i);
                const std::string switchedName = characters[i]->GetCharacterName();
                std::printf("[showcase] Switched to %s\n", switchedName.c_str());
                camera.Reset(characters[i]->position);
            }
        }

        if (ConsumePress(window.GetHandle(), GLFW_KEY_F1, f1Prev)) {
            dayNight.SetLoopPhase(GameLoopPhase::Explore);
            std::printf("[showcase] Game phase: %s\n", dayNight.GetPhaseName());
        }
        if (ConsumePress(window.GetHandle(), GLFW_KEY_F2, f2Prev)) {
            dayNight.SetLoopPhase(GameLoopPhase::Prepare);
            std::printf("[showcase] Game phase: %s\n", dayNight.GetPhaseName());
        }
        if (ConsumePress(window.GetHandle(), GLFW_KEY_F3, f3Prev)) {
            dayNight.SetLoopPhase(GameLoopPhase::Survive);
            std::printf("[showcase] Game phase: %s\n", dayNight.GetPhaseName());
        }
        if (ConsumePress(window.GetHandle(), GLFW_KEY_F4, f4Prev)) {
            dayNight.SetLoopPhase(GameLoopPhase::SwitchCharacter);
            std::printf("[showcase] Game phase: %s\n", dayNight.GetPhaseName());
        }
        if (ConsumePress(window.GetHandle(), GLFW_KEY_F5, f5Prev)) {
            dayNight.ToggleAutoAdvance();
            std::printf("[showcase] Auto phase loop %s\n", dayNight.IsAutoAdvanceEnabled() ? "enabled" : "disabled");
        }

        dayNight.Update(dt);

        UpdateCameraOrbit(camera, window.GetHandle(), dt, firstMouse, lastMouseX, lastMouseY);
        UpdateActiveCharacter(*characters[static_cast<std::size_t>(activeCharacterIndex)], camera, window.GetHandle(), dt);

        if (ConsumePress(window.GetHandle(), GLFW_KEY_SPACE, spacePrev)) {
            camera.Reset(characters[static_cast<std::size_t>(activeCharacterIndex)]->position);
        }

        for (std::size_t i = 0; i < characters.size(); ++i) {
            Character& character = *characters[i];
            character.Update(dt);
            character.position.y = world.GetHeightAt(character.position.x, character.position.z) - RootHeightBias(character);
            if (static_cast<int>(i) != activeCharacterIndex && character.state != CharacterState::DEAD) {
                character.state = CharacterState::IDLE;
            }
        }

        Character& followedCharacter = *characters[static_cast<std::size_t>(activeCharacterIndex)];
        camera.isUnderground = followedCharacter.position.y < -1.5f;
        camera.Update(followedCharacter.position, dt);
        const float cameraGround = world.GetHeightAt(camera.position.x, camera.position.z) + 0.8f;
        if (camera.position.y < cameraGround) {
            camera.position.y = glm::mix(camera.position.y, cameraGround, 0.45f);
        }

        renderer.BeginGeometryPass();
        world.DrawAll(renderer, camera, dayNight);

        const glm::mat4 view = camera.GetViewMatrix();
        const glm::mat4 projection = camera.GetProjectionMatrix(window.GetAspectRatio());

        characterShader.Bind();
        characterShader.SetMat4("view", view);
        characterShader.SetMat4("projection", projection);
        characterShader.SetFloat("roughness", 0.68f);
        characterShader.SetFloat("metalness", 0.02f);
        characterShader.SetVec3("emission", glm::vec3(0.0f));

        for (std::size_t i = 0; i < characters.size(); ++i) {
            Character& character = *characters[i];
            const float bob = (character.state == CharacterState::IDLE ? std::sin(time * 1.25f + static_cast<float>(i)) * 0.010f : 0.0f);
            glm::mat4 root(1.0f);
            root = glm::translate(root, character.position + glm::vec3(0.0f, bob, 0.0f));
            root = glm::rotate(root, character.facingAngle, glm::vec3(0.0f, 1.0f, 0.0f));

            ProceduralHumanoid* humanoid = character.GetProceduralHumanoid();
            if (!humanoid) {
                continue;
            }

            humanoid->UpdateJointTransform(JointId::ROOT, root);
            characterShader.SetVec3("albedoColor", humanoid->GetSkinColor());
            humanoid->Draw(characterShader, {});

            if (character.GetProceduralClothing()) {
                character.GetProceduralClothing()->Draw(characterShader, root);
            }
            if (character.GetProceduralHair()) {
                character.GetProceduralHair()->Draw(characterShader, root);
            }
            if (character.GetFaceDetails()) {
                character.GetFaceDetails()->Draw(characterShader, root);
            }
        }

        characterShader.Unbind();
        renderer.EndGeometryPass();

        std::vector<RenderCommand> emptyCommands;
        renderer.ShadowPass(emptyCommands, dayNight.GetSunDirection());
        renderer.SSAOPass();
        renderer.LightingPass(dayNight, camera);
        renderer.PostProcessPass(PostFXState{});

        window.SwapBuffers();
    }

    return 0;
}
