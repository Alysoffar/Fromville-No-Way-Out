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
        // Place NPCs near the town center so they are visible in the main play space.
        npcs.emplace_back("Mara", glm::vec3(-9.0f, 2.0f, 8.0f));
        npcs.emplace_back("Elena", glm::vec3(9.0f, 2.0f, 7.5f));
        npcs.emplace_back("Tom", glm::vec3(-8.5f, 2.0f, -7.0f));
        npcs.emplace_back("Kenny", glm::vec3(-34.0f, 2.0f, -34.0f));
        npcs.emplace_back("Kristi", glm::vec3(-10.0f, 2.0f, -2.0f));
        npcs.emplace_back("Ellis", glm::vec3(10.0f, 2.0f, 9.0f));
        npcs.emplace_back("Fatima", glm::vec3(8.0f, 2.0f, 9.0f));
        npcs.emplace_back("Donna", glm::vec3(7.0f, 2.0f, 7.0f));
    }
    EnsureNpcDialogueCooldowns();

    // Provide POIs for NPC wandering (story location centers)
    std::vector<glm::vec3> pois;
    for (const auto& loc : storyLocations) {
        pois.push_back(loc.center);
    }
    for (NPC& npc : npcs) {
        npc.SetPOIs(pois);
    }

    // Create enemies in the forest perimeter
    if (enemies.empty()) {
        // Keep enemies on the outskirts but close enough to see once night falls.
        enemies.emplace_back(glm::vec3(16.0f, 2.0f, 26.0f));
        enemies.emplace_back(glm::vec3(-18.0f, 2.0f, -24.0f));
        enemies.emplace_back(glm::vec3(22.0f, 2.0f, -20.0f));
        enemies.emplace_back(glm::vec3(-24.0f, 2.0f, 20.0f));
        enemies.emplace_back(glm::vec3(10.0f, 2.0f, -28.0f));
        enemies.emplace_back(glm::vec3(-15.0f, 2.0f, 26.0f));
        enemies.emplace_back(glm::vec3(25.0f, 2.0f, -14.0f));
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

    if (questSystem) {
        activeQuestCharacter = characters[activeCharacterIndex]->GetType();
        const Quest* activeQuest = questSystem->GetCharacterQuest(activeQuestCharacter);
        hasActiveQuest = activeQuest && !activeQuest->IsComplete() && !activeQuest->IsFailed();
    } else {
        hasActiveQuest = false;
    }

    hasPreviousActivePosition = false;
    previousActivePosition = characters[activeCharacterIndex]->transform.position;
}

int World::GetActiveCharacterIndex() const {
    return activeCharacterIndex;
}

Character* World::GetCharacter(int index) {
    if (index >= 0 && index < static_cast<int>(characters.size())) {
        return characters[index].get();
    }
    return nullptr;
}

bool World::LoadNextPendingMesh() {
    for (auto& character : characters) {
        if (character && !character->IsMeshLoaded()) {
            character->LoadDeferredMesh();
            return true;
        }
    }
    for (auto& npc : npcs) {
        if (!npc.IsMeshLoaded()) {
            npc.LoadDeferredMesh();
            return true;
        }
    }
    for (auto& enemy : enemies) {
        if (!enemy.IsMeshLoaded()) {
            enemy.LoadDeferredMesh();
            return true;
        }
    }
    return false;
}
