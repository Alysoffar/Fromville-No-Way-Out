#pragma once

#include <string>

#include "game/entities/BaseEntity.h"

class Character;

class Interactable : public BaseEntity {
public:
    explicit Interactable(std::string entityName = "Interactable");

    void Update(float dt) override;

    virtual std::string GetPrompt() const { return prompt; }
    virtual bool CanInteract(const Character& character) const;
    virtual void Interact(Character& character);

protected:
    std::string prompt = "Interact";
};
