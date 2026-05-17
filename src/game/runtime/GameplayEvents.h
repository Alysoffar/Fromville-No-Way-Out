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
