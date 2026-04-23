#include "EventBus.h"

#include <algorithm>

EventBus& EventBus::Get() {
    static EventBus instance;
    return instance;
}

int EventBus::Subscribe(GameEvent type, Callback cb) {
    const int id = nextSubId_++;
    listeners_[type].emplace_back(id, std::move(cb));
    return id;
}

void EventBus::Unsubscribe(int subscriptionId) {
    for (auto& [event, vec] : listeners_) {
        vec.erase(
            std::remove_if(vec.begin(), vec.end(),
                [subscriptionId](const std::pair<int, Callback>& p) {
                    return p.first == subscriptionId;
                }),
            vec.end());
    }
}

void EventBus::Fire(GameEvent type, PayloadMap payload) {
    EventData ev{ type, std::move(payload) };
    auto it = listeners_.find(type);
    if (it == listeners_.end()) {
        return;
    }
    // Copy the listener list so that subscriptions added inside a callback
    // don't invalidate our iteration.
    auto listenersCopy = it->second;
    for (auto& [id, cb] : listenersCopy) {
        cb(ev);
    }
}

void EventBus::Defer(GameEvent type, PayloadMap payload) {
    deferredQueue_.push_back(EventData{ type, std::move(payload) });
}

void EventBus::ProcessQueued() {
    if (processingDeferred_) {
        return;  // prevent re-entrancy
    }
    processingDeferred_ = true;

    // Swap out the queue so that events deferred during processing appear
    // in the NEXT frame, not the current one.
    std::vector<EventData> batch;
    batch.swap(deferredQueue_);

    for (const EventData& ev : batch) {
        auto it = listeners_.find(ev.type);
        if (it == listeners_.end()) {
            continue;
        }
        auto listenersCopy = it->second;
        for (auto& [id, cb] : listenersCopy) {
            cb(ev);
        }
    }

    processingDeferred_ = false;
}
