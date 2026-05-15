#pragma once

#include <string>
#include <utility>

#include "game/entities/Character.h"
#include "game/runtime/EventBus.h"

struct PuzzleCompletedEvent final : public GameplayEvent {
    PuzzleCompletedEvent() = default;
    PuzzleCompletedEvent(CharacterType characterIn, int objectiveIn)
        : character(characterIn), objectiveIndex(objectiveIn) {
    }

    CharacterType character = CharacterType::Boyd;
    int objectiveIndex = -1;
};

struct InteractionTriggeredEvent final : public GameplayEvent {
    InteractionTriggeredEvent() = default;
    InteractionTriggeredEvent(CharacterType characterIn, std::string interactionIdIn)
        : character(characterIn), interactionId(std::move(interactionIdIn)) {
    }

    CharacterType character = CharacterType::Boyd;
    std::string interactionId;
};

struct ActiveCharacterSwitchedEvent final : public GameplayEvent {
    ActiveCharacterSwitchedEvent() = default;
    ActiveCharacterSwitchedEvent(CharacterType fromIn, CharacterType toIn)
        : fromCharacter(fromIn), toCharacter(toIn) {
    }

    CharacterType fromCharacter = CharacterType::Boyd;
    CharacterType toCharacter = CharacterType::Boyd;
};

struct PromiseMadeEvent final : public GameplayEvent {
    PromiseMadeEvent() = default;
    PromiseMadeEvent(CharacterType characterIn, std::string promiseIdIn)
        : character(characterIn), promiseId(std::move(promiseIdIn)) {}
    CharacterType character = CharacterType::Boyd;
    std::string promiseId;
};

struct PromiseBrokenEvent final : public GameplayEvent {
    PromiseBrokenEvent() = default;
    PromiseBrokenEvent(CharacterType characterIn, std::string promiseIdIn)
        : character(characterIn), promiseId(std::move(promiseIdIn)) {}
    CharacterType character = CharacterType::Boyd;
    std::string promiseId;
};

struct AccusationEvent final : public GameplayEvent {
    AccusationEvent() = default;
    AccusationEvent(CharacterType characterIn, std::string accusationIdIn)
        : character(characterIn), accusationId(std::move(accusationIdIn)) {}
    CharacterType character = CharacterType::Boyd;
    std::string accusationId;
};
