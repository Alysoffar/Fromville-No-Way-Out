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

    // Create all 5 playable characters spread across the full map (±12 units)
    characters.emplace_back(std::make_unique<Boyd>(glm::vec3(-8.0f, 0.1f, 9.0f)));      // NW quadrant
    characters.emplace_back(std::make_unique<Jade>(glm::vec3(7.0f, 0.1f, 10.0f)));       // NE quadrant
    characters.emplace_back(std::make_unique<Tabitha>(glm::vec3(-9.5f, 0.1f, -8.0f)));   // SW quadrant
    characters.emplace_back(std::make_unique<Victor>(glm::vec3(8.5f, 0.1f, -9.5f)));     // SE quadrant
    characters.emplace_back(std::make_unique<Sara>(glm::vec3(0.0f, 0.1f, 0.0f)));        // Center

    // Create NPCs spread across the map
    if (npcs.empty()) {
        npcs.emplace_back("Mara", glm::vec3(-10.0f, 0.0f, 0.0f));      // West
        npcs.emplace_back("Elena", glm::vec3(10.0f, 0.0f, 2.0f));      // East
        npcs.emplace_back("Tom", glm::vec3(2.0f, 0.0f, -10.5f));       // South
    }
    EnsureNpcDialogueCooldowns();

    // Create enemies spread across the map perimeter
    if (enemies.empty()) {
        enemies.emplace_back(glm::vec3(10.5f, 0.0f, 8.0f));   // NE corner
        enemies.emplace_back(glm::vec3(-10.5f, 0.0f, -9.0f)); // SW corner
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

    // TODO: Move camera to new character position
    // TODO: Update HUD for new character stats
    // TODO: Apply post-processing effects (sepia for Victor memory mode, etc)
}
