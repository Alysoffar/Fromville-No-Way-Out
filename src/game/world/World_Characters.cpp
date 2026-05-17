#include "game/world/World.h"

#include <iostream>

#include "game/runtime/GameplayEvents.h"

Character* World::GetActiveCharacter() {
    return entityManager.GetActiveCharacter();
}

const Character* World::GetActiveCharacter() const {
    return entityManager.GetActiveCharacter();
}

void World::InitializeCharacters() {
    if (!characters.empty()) {
        return;
    }

    // Create all 5 playable characters spread across the terrain map
    characters.emplace_back(std::make_unique<Boyd>(glm::vec3(-20.0f, 2.0f, 15.0f)));
    characters.emplace_back(std::make_unique<Jade>(glm::vec3(20.0f, 2.0f, 15.0f)));
    characters.emplace_back(std::make_unique<Tabitha>(glm::vec3(-20.0f, 2.0f, -30.0f)));
    characters.emplace_back(std::make_unique<Victor>(glm::vec3(20.0f, 2.0f, -30.0f)));
    characters.emplace_back(std::make_unique<Sara>(glm::vec3(0.0f, 2.0f, 10.0f)));

    // Create NPCs spread across the map near buildings
    if (npcs.empty()) {
        npcs.emplace_back("Mara", glm::vec3(-22.0f, 0.0f, 5.0f));
        npcs.emplace_back("Elena", glm::vec3(22.0f, 0.0f, 5.0f));
        npcs.emplace_back("Tom", glm::vec3(5.0f, 0.0f, -35.0f));
    }
    EnsureNpcDialogueCooldowns();

    // Create enemies in the forest perimeter
    if (enemies.empty()) {
        enemies.emplace_back(glm::vec3(40.0f, 0.0f, 30.0f));
        enemies.emplace_back(glm::vec3(-40.0f, 0.0f, -40.0f));
    }
}

void World::SwitchCharacter(int index) {
    if (index < 0 || index >= static_cast<int>(characters.size())) {
        std::cerr << "[World] Invalid character index: " << index << "\n";
        return;
    }

    if (index == activeCharacterIndex) {
        return;  // Already active
    }

    const int previousIndex = activeCharacterIndex;

    // Call OnSwitchedFrom on previous character
    characters[activeCharacterIndex]->OnSwitchedFrom();
    std::cout << "[World] Switched from " << characters[activeCharacterIndex]->GetName() << "\n";

    // Switch to new character
    entityManager.SwitchActiveCharacter(index);
    characters[activeCharacterIndex]->OnSwitchedTo();
    std::cout << "[World] Switched to " << characters[activeCharacterIndex]->GetName() << "\n";

    eventBus.Publish(ActiveCharacterSwitchedEvent{
        characters[previousIndex]->GetType(),
        characters[activeCharacterIndex]->GetType()
    });

    hasPreviousActivePosition = false;
    previousActivePosition = characters[activeCharacterIndex]->transform.position;
}
