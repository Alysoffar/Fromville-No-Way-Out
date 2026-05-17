#pragma once

#include <functional>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

struct GameplayEvent {
    virtual ~GameplayEvent() = default;
};

class EventBus {
public:
    template <typename EventT>
    using Handler = std::function<void(const EventT&)>;

    template <typename EventT>
    void Subscribe(Handler<EventT> handler) {
        auto& bucket = handlers[std::type_index(typeid(EventT))];
        bucket.push_back([fn = std::move(handler)](const GameplayEvent& evt) {
            fn(static_cast<const EventT&>(evt));
        });
    }

    template <typename EventT>
    void Publish(const EventT& event) const {
        auto it = handlers.find(std::type_index(typeid(EventT)));
        if (it == handlers.end()) {
            return;
        }

        for (const auto& handler : it->second) {
            handler(event);
        }
    }

private:
    using ErasedHandler = std::function<void(const GameplayEvent&)>;
    std::unordered_map<std::type_index, std::vector<ErasedHandler>> handlers;
};
