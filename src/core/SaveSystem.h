#pragma once

#include <string>
#include <vector>

class CharacterManager;
class NarrativeEngine;
class World;
class DayNightCycle;
class NPC;

class SaveSystem {
public:
    static SaveSystem& Get();
    bool SaveGame(const std::string& slot,
                  CharacterManager& cm,
                  NarrativeEngine&  ne,
                  World&            world,
                  DayNightCycle&    dayNight,
                  std::vector<NPC*>& npcs);
    bool LoadGame(const std::string& slot,
                  CharacterManager& cm,
                  NarrativeEngine&  ne,
                  World&            world,
                  DayNightCycle&    dayNight,
                  std::vector<NPC*>& npcs);
    bool AutoSaveExists() const;
    std::vector<std::string> GetSaveSlots() const;

private:
    std::string GetSavePath(const std::string& slot) const;
};
