#include "game/world/EntityManager.h"

#include "engine/physics/CollisionWorld.h"

void EntityManager::BindStorage(std::vector<std::unique_ptr<Character>>* worldCharacters,
                                std::vector<NPC>* worldNpcs,
                                std::vector<Enemy>* worldEnemies,
                                int* worldActiveCharacterIndex) {
    characters = worldCharacters;
    npcs = worldNpcs;
    enemies = worldEnemies;
    activeCharacterIndex = worldActiveCharacterIndex;
}

void EntityManager::BindCollisionWorld(CollisionWorld* world) {
    if (!characters || !npcs || !enemies) {
        return;
    }

    for (auto& character : *characters) {
        character->SetCollisionWorld(world);
    }
    for (NPC& npc : *npcs) {
        npc.SetCollisionWorld(world);
    }
    for (Enemy& enemy : *enemies) {
        enemy.SetCollisionWorld(world);
    }
}

Character* EntityManager::GetActiveCharacter() {
    if (!characters || !activeCharacterIndex) {
        return nullptr;
    }
    if (*activeCharacterIndex < 0 || *activeCharacterIndex >= static_cast<int>(characters->size())) {
        return nullptr;
    }
    return (*characters)[*activeCharacterIndex].get();
}

const Character* EntityManager::GetActiveCharacter() const {
    if (!characters || !activeCharacterIndex) {
        return nullptr;
    }
    if (*activeCharacterIndex < 0 || *activeCharacterIndex >= static_cast<int>(characters->size())) {
        return nullptr;
    }
    return (*characters)[*activeCharacterIndex].get();
}

bool EntityManager::SwitchActiveCharacter(int index) {
    if (!characters || !activeCharacterIndex) {
        return false;
    }

    if (index < 0 || index >= static_cast<int>(characters->size()) || index == *activeCharacterIndex) {
        return false;
    }

    *activeCharacterIndex = index;
    return true;
}
