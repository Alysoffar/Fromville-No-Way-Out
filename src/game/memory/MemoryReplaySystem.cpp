#include "game/memory/MemoryReplaySystem.h"

#include <algorithm>
#include <cmath>

namespace {
static MemoryReplaySystem* g_memorySystem = nullptr;
}

MemoryReplaySystem& MemoryReplaySystem::Instance() {
    if (!g_memorySystem) {
        g_memorySystem = new MemoryReplaySystem();
    }
    return *g_memorySystem;
}

MemoryReplaySystem::MemoryReplaySystem()
    : memoryModeActive(false), traumaLevel(0.3f), vhsDistortion(0.0f) {
    // Predefine key memory events from Victor's timeline
    RegisterMemoryEvent({
        MemoryEventType::Disappearance,
        "1985: Sarah vanishes from school",
        glm::vec3(2.0f, 3.0f, 0.0f),
        1985,
        0,
        false,
        false,
        0.8f
    });

    RegisterMemoryEvent({
        MemoryEventType::Dialogue,
        "1990: Victor sees the cult leader recruiting Margaret",
        glm::vec3(1.0f, 2.0f, 0.0f),
        1990,
        1,
        false,
        false,
        0.7f
    });

    RegisterMemoryEvent({
        MemoryEventType::Ritual,
        "1995: Victor witnesses chanting in the cemetery chapel",
        glm::vec3(0.0f, 0.0f, 0.0f),
        1995,
        2,
        false,
        false,
        0.9f
    });

    RegisterMemoryEvent({
        MemoryEventType::Betrayal,
        "2000: The leader offers Victor a place in the cult",
        glm::vec3(-1.0f, 1.0f, 0.0f),
        2000,
        3,
        false,
        false,
        0.6f
    });

    RegisterMemoryEvent({
        MemoryEventType::Discovery,
        "Now: Sarah returns — she is the survivor",
        glm::vec3(3.0f, -1.0f, 0.0f),
        2025,
        4,
        false,
        false,
        0.5f
    });
}

void MemoryReplaySystem::Update(float dt) {
    if (memoryModeActive) {
        memoryModeTimer -= dt;
        if (memoryModeTimer <= 0.0f) {
            memoryModeActive = false;
            memoryModeIntensity = 0.0f;
        } else {
            memoryModeIntensity = 1.0f - (memoryModeTimer / memoryModeDuration);
        }
    }

    if (vhsDistortion > 0.0f) {
        vhsDistortion = std::max(0.0f, vhsDistortion - dt * 0.6f);
    }

    for (auto it = activeGhosts.begin(); it != activeGhosts.end();) {
        it->duration -= dt;
        if (it->duration <= 0.0f) {
            it = activeGhosts.erase(it);
        } else {
            ++it;
        }
    }
}

void MemoryReplaySystem::Render() const {
}

void MemoryReplaySystem::RegisterMemoryEvent(const MemoryEvent& event) {
    memoryEvents.push_back(event);
    eventMap[event.description] = event;
}

void MemoryReplaySystem::ActivateMemoryMode(float duration) {
    memoryModeActive = true;
    memoryModeDuration = duration;
    memoryModeTimer = duration;
    memoryModeIntensity = 0.0f;
    vhsDistortion = 1.0f;

    for (auto& event : memoryEvents) {
        if (event.witnessed || event.traumaWeight > 0.6f) {
            witnessedEvents.push_back(event);
        }
    }
}

void MemoryReplaySystem::UpdateMemoryMode(float dt) {
    if (memoryModeActive) {
        vhsDistortion = std::sin(memoryModeTimer * 6.0f) * 0.3f + 0.4f;
    }
}

bool MemoryReplaySystem::CheckTimelineOrder(const std::vector<std::string>& playerOrder) {
    std::vector<std::string> correctOrder;
    for (const auto& event : memoryEvents) {
        if (event.sortOrder >= 0) {
            correctOrder.push_back(event.description);
        }
    }

    std::sort(correctOrder.begin(), correctOrder.end(), [this](const auto& a, const auto& b) {
        return eventMap.at(a).sortOrder < eventMap.at(b).sortOrder;
    });

    return playerOrder == correctOrder;
}

std::string MemoryReplaySystem::ReconstructMemory(const std::vector<std::string>& order) {
    if (CheckTimelineOrder(order)) {
        return "The timeline becomes clear. Victor remembers the whole terrible sequence.";
    } else {
        traumaLevel += 0.1f;
        return "The memories refuse to cohere. Victor's mind shields itself.";
    }
}

void MemoryReplaySystem::TriggerGhostReplica(const std::string& characterName, const std::string& dialogue, const glm::vec3& location) {
    GhostReplica ghost{characterName, location, dialogue, 3.0f, true};
    activeGhosts.push_back(ghost);
}

void MemoryReplaySystem::ProcessTraumaticMemory(const std::string& eventName) {
    auto it = eventMap.find(eventName);
    if (it != eventMap.end()) {
        traumaLevel = std::clamp(traumaLevel + it->second.traumaWeight, 0.0f, 1.0f);
    }
}

void MemoryReplaySystem::DistortMemory(const std::string& eventName) {
    auto it = eventMap.find(eventName);
    if (it != eventMap.end()) {
        it->second.distorted = true;
    }
}

bool MemoryReplaySystem::IsMemoryDistorted(const std::string& eventName) const {
    auto it = eventMap.find(eventName);
    if (it != eventMap.end()) {
        return it->second.distorted;
    }
    return false;
}

bool MemoryReplaySystem::CheckForContradiction(const std::string& eventA, const std::string& eventB) const {
    auto itA = eventMap.find(eventA);
    auto itB = eventMap.find(eventB);
    if (itA == eventMap.end() || itB == eventMap.end()) return false;
    if (itA->second.location == itB->second.location && itA->second.year == itB->second.year) return true;
    return false;
}

std::string MemoryReplaySystem::GetContradictionInsight(const std::string& eventA, const std::string& eventB) const {
    if (CheckForContradiction(eventA, eventB)) {
        return "Victor realizes someone was lying about their whereabouts.";
    }
    return "No contradiction found.";
}
