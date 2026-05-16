#include "game/atmosphere/AtmosphereManager.h"

#include <algorithm>
#include <cmath>
#include <random>

#include "engine/renderer/TextRenderer.h"

namespace {
static AtmosphereManager* g_atmosphere = nullptr;

const std::array<const char*, 8> kChildPhrases = {{
    "anghkooey...",
    "mother is watching",
    "the lights go out",
    "we are still here",
    "come sing with us",
    "the basement remembers",
    "they took the children",
    "don't look away"
}};
}

AtmosphereManager& AtmosphereManager::Instance() {
    if (!g_atmosphere) {
        g_atmosphere = new AtmosphereManager();
    }
    return *g_atmosphere;
}

AtmosphereManager::AtmosphereManager()
    : globalTension(0.3f), corruptionLevel(0.0f), ambientWhisperTimer(0.0f) {
}

void AtmosphereManager::Update(float dt) {
    UpdateWhispers(dt);
    UpdateHallucinations(dt);
    UpdateAmbientEffects(dt);

    if (corruptionLevel > 0.5f && globalTension > 0.6f) {
        IncreaseTension(dt * 0.05f);
    }
}

void AtmosphereManager::UpdateWhispers(float dt) {
    for (auto it = pendingWhispers.begin(); it != pendingWhispers.end();) {
        it->timer -= dt;
        it->intensity = std::clamp(it->intensity + dt * 1.2f, 0.0f, 1.0f);
        if (it->timer <= 0.0f) {
            it = pendingWhispers.erase(it);
        } else {
            ++it;
        }
    }

    ambientWhisperTimer -= dt;
    if (ambientWhisperTimer <= 0.0f) {
        const float nextInterval = 12.0f + (globalTension * 10.0f);
        ambientWhisperTimer = nextInterval;

        if (corruptionLevel > 0.38f && globalTension > 0.45f) {
            static std::mt19937 rng(std::random_device{}());
            std::uniform_int_distribution<> dist(0, static_cast<int>(kChildPhrases.size() - 1));
            if ((dist(rng) % 3) == 0) {
                const char* phrase = kChildPhrases[dist(rng)];
                TriggerChildWhisper(phrase, glm::vec3(0.0f));
            }
        }
    }
}

void AtmosphereManager::UpdateHallucinations(float dt) {
    for (auto it = hallucinations.begin(); it != hallucinations.end();) {
        it->timer -= dt;
        if (it->timer > it->duration * 0.5f) {
            it->intensity = std::clamp(it->intensity + dt * 2.0f, 0.0f, 1.0f);
        } else {
            it->intensity = std::clamp(it->intensity - dt * 2.5f, 0.0f, 1.0f);
        }

        if (it->timer <= 0.0f) {
            it = hallucinations.erase(it);
        } else {
            ++it;
        }
    }

    hallucinationActive = !hallucinations.empty();
    hallucinationIntensity = 0.0f;
    for (const auto& h : hallucinations) {
        hallucinationIntensity = std::max(hallucinationIntensity, h.intensity);
    }
}

void AtmosphereManager::UpdateAmbientEffects(float dt) {
    if (symbolPulseIntensity > 0.0f) {
        symbolPulseTimer -= dt;
        symbolPulseIntensity = std::max(0.0f, symbolPulseIntensity - dt * 0.8f);
        if (symbolPulseTimer <= 0.0f) {
            symbolPulseTimer = 0.0f;
        }
    }
}

void AtmosphereManager::Render(TextRenderer& textRenderer, int screenWidth, int screenHeight) const {
    RenderHallucinations(textRenderer, screenWidth, screenHeight);
}

void AtmosphereManager::RenderHallucinations(TextRenderer& textRenderer, int screenWidth, int screenHeight) const {
    for (const auto& h : hallucinations) {
        if (h.intensity < 0.01f) continue;

        const glm::vec3 color = glm::vec3(1.0f, 0.3f, 0.3f) * (0.5f + 0.5f * std::sin(h.timer * 6.0f));
        const float baseY = static_cast<float>(screenHeight) * 0.4f;
        const float jitter = std::sin(h.timer * 12.0f) * 4.0f;

        textRenderer.RenderText(
            h.text,
            static_cast<float>(screenWidth) * 0.5f - 200.0f + jitter,
            baseY,
            1.2f,
            color * h.intensity,
            screenWidth,
            screenHeight
        );
    }

    // Render pending whispers at screen edges
    for (const auto& w : pendingWhispers) {
        const glm::vec3 whisperColor = glm::vec3(0.6f, 0.8f, 1.0f) * (0.3f + 0.7f * w.intensity);
        const float edge = static_cast<float>(screenWidth) * (0.1f + w.intensity * 0.3f);
        const float yPos = static_cast<float>(screenHeight) * 0.3f + std::sin(w.timer * 4.0f) * 20.0f;

        textRenderer.RenderText(
            "... " + w.phrase + " ...",
            edge,
            yPos,
            0.42f,
            whisperColor * (0.4f + 0.6f * w.intensity),
            screenWidth,
            screenHeight
        );
    }
}

void AtmosphereManager::TriggerEvent(AtmosphereEvent event, const glm::vec3& position, float intensity) {
    switch (event) {
        case AtmosphereEvent::ChildWhisper:
            TriggerChildWhisper("...", position);
            break;
        case AtmosphereEvent::DistantSinging:
            IncreaseTension(0.08f);
            break;
        case AtmosphereEvent::WallBleeding:
            IncreaseTension(0.12f);
            symbolPulseIntensity = intensity;
            symbolPulseTimer = 1.5f;
            break;
        case AtmosphereEvent::SymbolRearrangement:
            symbolPulseIntensity = intensity * 1.5f;
            symbolPulseTimer = 2.0f;
            break;
        case AtmosphereEvent::CorruptionPulse:
            SetCorruption(corruptionLevel + intensity * 0.1f);
            IncreaseTension(intensity * 0.15f);
            break;
        default:
            break;
    }
}

void AtmosphereManager::TriggerChildWhisper(const std::string& phrase, const glm::vec3& position) {
    PendingWhisper w;
    w.phrase = phrase;
    w.position = position;
    w.timer = 2.0f + (corruptionLevel * 1.5f);
    w.intensity = 0.0f;
    pendingWhispers.push_back(w);
    IncreaseTension(0.06f);
}

void AtmosphereManager::TriggerHallucination(const std::string& text, float duration) {
    ActiveHallucination h;
    h.text = text;
    h.duration = duration;
    h.timer = duration;
    h.intensity = 0.0f;
    hallucinations.push_back(h);
    IncreaseTension(0.1f);
}

void AtmosphereManager::TriggerSymbolPulse(float intensity) {
    symbolPulseIntensity = std::max(symbolPulseIntensity, intensity);
    symbolPulseTimer = 1.2f;
}

void AtmosphereManager::TriggerMemoryFlash(const std::string& imagery) {
    TriggerHallucination("[" + imagery + "]", 2.0f);
    IncreaseTension(0.1f);
}
