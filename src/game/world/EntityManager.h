#pragma once

#include <array>
#include <memory>
#include <vector>

#include "game/entities/Boyd.h"
#include "game/entities/Enemy.h"
#include "game/entities/Jade.h"
#include "game/entities/NPC.h"
#include "game/entities/Sara.h"
#include "game/entities/Tabitha.h"
#include "game/entities/Victor.h"

class CollisionWorld;

class EntityManager {
public:
    void BindStorage(std::vector<std::unique_ptr<Character>>* worldCharacters,
                     std::vector<NPC>* worldNpcs,
                     std::vector<Enemy>* worldEnemies,
                     int* worldActiveCharacterIndex);
    void BindCollisionWorld(CollisionWorld* world);

    Character* GetActiveCharacter();
    const Character* GetActiveCharacter() const;
    int GetActiveCharacterIndex() const { return activeCharacterIndex ? *activeCharacterIndex : -1; }
    bool SwitchActiveCharacter(int index);

    std::vector<std::unique_ptr<Character>>& Characters() { return *characters; }
    const std::vector<std::unique_ptr<Character>>& Characters() const { return *characters; }
    std::vector<NPC>& Npcs() { return *npcs; }
    const std::vector<NPC>& Npcs() const { return *npcs; }
    std::vector<Enemy>& Enemies() { return *enemies; }
    const std::vector<Enemy>& Enemies() const { return *enemies; }

private:
    std::vector<std::unique_ptr<Character>>* characters = nullptr;
    std::vector<NPC>* npcs = nullptr;
    std::vector<Enemy>* enemies = nullptr;
    int* activeCharacterIndex = nullptr;
};
