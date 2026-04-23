#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include <glm/glm.hpp>

// ---------------------------------------------------------------------------
// GameEvent — all game-wide events that can be fired through the bus
// ---------------------------------------------------------------------------
enum class GameEvent {
    TALISMAN_PLACED,
    TALISMAN_REMOVED,
    ELDER_TALISMAN_EXPIRED,

    NPC_OPENED_DOOR,
    NPC_INTERROGATED,
    NPC_DIED,

    GLYPH_DECODED,
    SYMBOL_SIGHT_ACTIVATED,

    TUNNEL_SECTION_REVEALED,
    TABITHA_NIGHT_TUNNEL_WARNING,

    BOYD_RAGE_START,
    BOYD_RAGE_END,

    JADE_VISION_TRIGGER,

    VICTOR_MEMORY_MODE,

    SARA_GHOST_STEP_START,
    SARA_REDEMPTION_CHANGED,

    CHARACTER_SWITCH_REQUESTED,
    CHARACTER_SWITCHED,

    CREATURE_SPAWNED,
    CREATURE_DIED,

    PLAYER_DAMAGED,
    PLAYER_DIED,

    QUEST_UPDATED,
    QUEST_COMPLETED,

    DIALOGUE_STARTED,
    DIALOGUE_ENDED,

    SAVE_REQUESTED,
    LOAD_REQUESTED,

    DAWN_BEGAN,
    DUSK_BEGAN,
    NIGHT_BEGAN
};

// ---------------------------------------------------------------------------
// EventData — typed payload using std::variant values
// ---------------------------------------------------------------------------
using PayloadValue = std::variant<int, float, bool, std::string, glm::vec3>;
using PayloadMap   = std::unordered_map<std::string, PayloadValue>;

struct EventData {
    GameEvent  type;
    PayloadMap payload;
};

// ---------------------------------------------------------------------------
// EventBus — synchronous subscription + optional per-frame deferred flush
// ---------------------------------------------------------------------------
class EventBus {
public:
    static EventBus& Get();

    using Callback = std::function<void(const EventData&)>;

    /// Subscribe to events of the given type.
    /// Returns a subscription ID that can be passed to Unsubscribe().
    int  Subscribe(GameEvent type, Callback cb);

    /// Remove a previously registered subscription.
    void Unsubscribe(int subscriptionId);

    /// Fire an event immediately, invoking all listeners synchronously.
    void Fire(GameEvent type, PayloadMap payload = {});

    /// Queue an event to be processed the next time ProcessQueued() is called.
    void Defer(GameEvent type, PayloadMap payload = {});

    /// Flush all deferred events (call once per frame).
    void ProcessQueued();

private:
    EventBus() = default;

    std::unordered_map<GameEvent,
        std::vector<std::pair<int, Callback>>> listeners_;

    std::vector<EventData> deferredQueue_;
    bool processingDeferred_ = false;   // re-entrancy guard

    int nextSubId_ = 0;
};
