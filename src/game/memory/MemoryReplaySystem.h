#pragma once

#include <array>
#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

enum class MemoryEventType {
    Dialogue,
    ViolentAct,
    Betrayal,
    Disappearance,
    Discovery,
    Ritual,
    Trauma
};

struct MemoryEvent {
    MemoryEventType type;
    std::string description;
    glm::vec3 location;
    int year = 0;
    int sortOrder = -1;
    bool witnessed = false;
    bool distorted = false;
    float traumaWeight = 0.5f;
};

struct MemoryTimeline {
    std::vector<std::string> correctOrder;
    std::vector<std::string> scrambledOrder;
    std::string interpretation;
};

struct GhostReplica {
    std::string characterName;
    glm::vec3 position;
    std::string dialogue;
    float duration = 0.0f;
    bool visible = false;
};

class MemoryReplaySystem {
public:
    static MemoryReplaySystem& Instance();

    void Update(float dt);
    void Render() const;

    // Register memory events from Victor's past
    void RegisterMemoryEvent(const MemoryEvent& event);

    // Victor's Memory Mode ability
    void ActivateMemoryMode(float duration);
    void UpdateMemoryMode(float dt);
    bool IsMemoryModeActive() const { return memoryModeActive; }
    float GetMemoryModeIntensity() const { return memoryModeIntensity; }

    // Timeline reconstruction puzzle
    const std::vector<MemoryEvent>& GetMemoryEvents() const { return memoryEvents; }
    const std::vector<MemoryEvent>& GetWitnessedEvents() const { return witnessedEvents; }
    bool CheckTimelineOrder(const std::vector<std::string>& playerOrder);
    std::string ReconstructMemory(const std::vector<std::string>& order);

    // Ghost events (replays)
    void TriggerGhostReplica(const std::string& characterName, const std::string& dialogue, const glm::vec3& location);
    const std::vector<GhostReplica>& GetActiveGhosts() const { return activeGhosts; }

    // Trauma system
    float GetTraumaLevel() const { return traumaLevel; }
    void IncraseTrauma(float amount) { traumaLevel = std::clamp(traumaLevel + amount, 0.0f, 1.0f); }
    void ProcessTraumaticMemory(const std::string& eventName);

    // Memory distortion
    void DistortMemory(const std::string& eventName);
    bool IsMemoryDistorted(const std::string& eventName) const;

    // Dialogue contradiction system
    bool CheckForContradiction(const std::string& eventA, const std::string& eventB) const;
    std::string GetContradictionInsight(const std::string& eventA, const std::string& eventB) const;

private:
    MemoryReplaySystem();
    ~MemoryReplaySystem() = default;

    std::vector<MemoryEvent> memoryEvents;
    std::vector<MemoryEvent> witnessedEvents;
    std::unordered_map<std::string, MemoryEvent> eventMap;
    std::vector<GhostReplica> activeGhosts;

    bool memoryModeActive = false;
    float memoryModeTimer = 0.0f;
    float memoryModeDuration = 0.0f;
    float memoryModeIntensity = 0.0f;

    float traumaLevel = 0.0f;
    float vhsDistortion = 0.0f;

    const std::array<const char*, 4> kTraumaResponses = {{
        "The memory shatters.",
        "Victor cannot face this.",
        "The past refuses to surface.",
        "Forgetting was easier."
    }};
};
