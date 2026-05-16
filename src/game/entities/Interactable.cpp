#include "game/entities/Interactable.h"

#include "game/entities/Character.h"

Interactable::Interactable(std::string entityName)
    : BaseEntity(std::move(entityName)) {
}

void Interactable::Update(float dt) {
    (void)dt;
}

bool Interactable::CanInteract(const Character& character) const {
    (void)character;
    return IsActive();
}

void Interactable::Interact(Character& character) {
    (void)character;
}
