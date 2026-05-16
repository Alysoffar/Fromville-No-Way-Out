#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>

class TextRenderer;

enum class AtmosphereEvent {
    ChildWhisper,
    DistantSinging,
    WallBleeding,
    ShadowMovement,
    MemoryFlash,
    CorruptionPulse,
    HallucinationOverlay,
    SymbolRearrangement
};

class AtmosphereManager {
public:
    static AtmosphereManager& Instance();

    void Update(float dt);
    void Render(TextRenderer& textRenderer, int screenWidth, int screenHeight) const;

    // Trigger atmosphere events
    void TriggerEvent(AtmosphereEvent event, const glm::vec3& position = glm::vec3(0.0f), float intensity = 1.0f);
    void TriggerChildWhisper(const std::string& phrase, const glm::vec3& position);
    void TriggerHallucination(const std::string& text, float duration);
    void TriggerSymbolPulse(float intensity);
    void TriggerMemoryFlash(const std::string& imagery);

    // Global state
    float GetGlobalTension() const { return globalTension; }
    void SetGlobalTension(float value) { globalTension = glm::clamp(value, 0.0f, 1.0f); }
    void IncreaseTension(float amount) { globalTension = glm::clamp(globalTension + amount, 0.0f, 1.0f); }

    float GetCorruptionLevel() const { return corruptionLevel; }
    void SetCorruption(float value) { corruptionLevel = glm::clamp(value, 0.0f, 1.0f); }

    bool IsHallucinating() const { return hallucinationActive; }
    float GetHallucinationIntensity() const { return hallucinationIntensity; }

private:
    AtmosphereManager();
    ~AtmosphereManager() = default;

    struct PendingWhisper {
        std::string phrase;
        glm::vec3 position;
        float timer = 2.0f;
        float intensity = 0.0f;
    };

    struct ActiveHallucination {
        std::string text;
        float timer = 0.0f;
        float duration = 0.0f;
        float intensity = 0.0f;
    };

    float globalTension = 0.3f;
    float corruptionLevel = 0.0f;
    float ambientWhisperTimer = 0.0f;
    float symbolPulseIntensity = 0.0f;
    float symbolPulseTimer = 0.0f;

    bool hallucinationActive = false;
    float hallucinationIntensity = 0.0f;
    std::vector<ActiveHallucination> hallucinations;
    std::vector<PendingWhisper> pendingWhispers;

    void UpdateWhispers(float dt);
    void UpdateHallucinations(float dt);
    void UpdateAmbientEffects(float dt);
    void RenderHallucinations(TextRenderer& textRenderer, int screenWidth, int screenHeight) const;
};
