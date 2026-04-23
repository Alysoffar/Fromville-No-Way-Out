#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

#include "core/EventBus.h"

class Boyd;
class Character;
class EventBus;
class QuestLog;
class Sara;
class Victor;

// ---------------------------------------------------------------------------
// NarrativeEngine — owns the world-state flag map and reacts to game events
//                   by mutating character state and advancing the quest log.
// ---------------------------------------------------------------------------
class NarrativeEngine {
public:
    NarrativeEngine(QuestLog* quests, EventBus* bus);

    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------

    /// Register all EventBus subscriptions and load initial quests.
    /// Call after all characters and the EventBus are ready.
    void Init(std::vector<Character*>& characters);

    /// Forward EventBus::ProcessQueued() — call once per game frame.
    void ProcessEvents();

    // -----------------------------------------------------------------------
    // Flag API (used by DialogueSystem, QuestLog, consequence handlers)
    // -----------------------------------------------------------------------

    void SetFlag(const std::string& flag, bool value = true);
    bool GetFlag(const std::string& flag) const;

    const std::unordered_map<std::string, bool>& GetAllFlags() const;

    // -----------------------------------------------------------------------
    // Quest bootstrapping
    // -----------------------------------------------------------------------

    /// Parse a JSON file and populate the QuestLog.
    /// Expected format: array of quest objects matching the Quest struct.
    void LoadQuestsFromJSON(const std::string& path);

private:
    QuestLog*              questLog_;
    EventBus*              bus_;
    std::vector<Character*> characters_;

    std::unordered_map<std::string, bool> flags_;

    // Subscription IDs (held so we can Unsubscribe in destructor if needed)
    std::vector<int> subscriptionIds_;

    // -----------------------------------------------------------------------
    // Consequence handlers — registered in Init()
    // -----------------------------------------------------------------------
    void OnNPCDied(const EventData& e);
    void OnGlyphDecoded(const EventData& e);
    void OnTalismanPlaced(const EventData& e);
    void OnNPCOpenedDoor(const EventData& e);
    void OnBoydRageStart(const EventData& e);
    void OnSaraRedemptionChanged(const EventData& e);
    void OnDawnBegan(const EventData& e);

    // -----------------------------------------------------------------------
    // Character lookups (safe — return nullptr if not found)
    // -----------------------------------------------------------------------
    Sara*   GetSara()   const;
    Boyd*   GetBoyd()   const;
    Victor* GetVictor() const;
};
