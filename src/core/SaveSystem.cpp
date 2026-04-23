#include "SaveSystem.h"

#include <fstream>
#include <chrono>
#include <filesystem>
#include <iostream>

#include <nlohmann/json.hpp>

#include "characters/CharacterManager.h"
#include "narrative/NarrativeEngine.h"
#include "world/DayNightCycle.h"
#include "ai/NPC.h"
#include "world/World.h"

#include "characters/Boyd.h"
#include "characters/Jade.h"
#include "characters/Tabitha.h"
#include "characters/Victor.h"
#include "characters/Sara.h"

SaveSystem& SaveSystem::Get() {
    static SaveSystem instance;
    return instance;
}

std::string SaveSystem::GetSavePath(const std::string& slot) const {
    return "saves/" + slot + ".json";
}

bool SaveSystem::AutoSaveExists() const {
    return std::filesystem::exists(GetSavePath("autosave"));
}

std::vector<std::string> SaveSystem::GetSaveSlots() const {
    std::vector<std::string> slots;
    if (std::filesystem::exists("saves")) {
        for (const auto& entry : std::filesystem::directory_iterator("saves")) {
            if (entry.path().extension() == ".json") {
                slots.push_back(entry.path().stem().string());
            }
        }
    }
    return slots;
}

bool SaveSystem::SaveGame(const std::string& slot, CharacterManager& cm, NarrativeEngine& ne, World& world, DayNightCycle& dayNight, std::vector<NPC*>& npcs) {
    if (!std::filesystem::exists("saves")) {
        std::filesystem::create_directory("saves");
    }

    nlohmann::json j;
    j["version"] = 1;
    
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    char buf[100];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&now_time));
    j["timestamp"] = std::string(buf);
    
    j["timeOfDay"] = dayNight.GetTimeOfDay();
    j["activeCharacter"] = cm.GetActive() ? cm.GetActive()->GetCharacterName() : "";
    
    j["questFlags"] = ne.GetAllFlags();

    nlohmann::json charsJson;
    for (Character* c : cm.GetAll()) {
        nlohmann::json cj;
        cj["position"] = {c->position.x, c->position.y, c->position.z};
        cj["health"] = c->stats.health;
        cj["stamina"] = c->stats.stamina;

        if (auto* boyd = dynamic_cast<Boyd*>(c)) {
            cj["curseMeter"] = boyd->curseMeter;
            cj["revolverAmmo"] = boyd->revolverAmmo;
            cj["reserveAmmo"] = boyd->reserveAmmo;
            charsJson["Boyd"] = cj;
        } else if (auto* jade = dynamic_cast<Jade*>(c)) {
            cj["visionTimer"] = jade->visionTimer;
            cj["decodedGlyphs"] = jade->decodedGlyphs;
            charsJson["Jade"] = cj;
        } else if (auto* tabitha = dynamic_cast<Tabitha*>(c)) {
            cj["revealedTunnelSections"] = tabitha->revealedTunnelSections;
            charsJson["Tabitha"] = cj;
        } else if (auto* victor = dynamic_cast<Victor*>(c)) {
            cj["talismansCarried"] = victor->talismansCarried;
            cj["elderTalismansCarried"] = victor->elderTalismansCarried;
            charsJson["Victor"] = cj;
        } else if (auto* sara = dynamic_cast<Sara*>(c)) {
            cj["redemptionScore"] = sara->redemptionScore;
            charsJson["Sara"] = cj;
        }
    }
    j["characters"] = charsJson;

    nlohmann::json npcsJson = nlohmann::json::array();
    for (NPC* npc : npcs) {
        nlohmann::json nj;
        nj["id"] = npc->id;
        nj["name"] = npc->name;
        nj["health"] = npc->health;
        nj["position"] = {npc->position.x, npc->position.y, npc->position.z};
        nj["alive"] = npc->alive;
        npcsJson.push_back(nj);
    }
    j["npcs"] = npcsJson;

    // Stubs for systems not fully accessible
    j["talismans"] = nlohmann::json::array();
    j["creatures"] = nlohmann::json::array();

    std::ofstream file(GetSavePath(slot));
    if (!file.is_open()) return false;
    file << j.dump(2);
    return true;
}

bool SaveSystem::LoadGame(const std::string& slot, CharacterManager& cm, NarrativeEngine& ne, World& world, DayNightCycle& dayNight, std::vector<NPC*>& npcs) {
    std::ifstream file(GetSavePath(slot));
    if (!file.is_open()) return false;

    nlohmann::json j;
    try {
        file >> j;
    } catch (...) {
        return false;
    }

    if (j.contains("timeOfDay")) {
        dayNight.SetTime(j["timeOfDay"].get<float>());
    }

    if (j.contains("questFlags")) {
        for (auto& [key, value] : j["questFlags"].items()) {
            ne.SetFlag(key, value.get<bool>());
        }
    }

    if (j.contains("characters")) {
        auto charsJson = j["characters"];
        for (Character* c : cm.GetAll()) {
            std::string name = c->GetCharacterName();
            if (charsJson.contains(name)) {
                auto cj = charsJson[name];
                if (cj.contains("position")) {
                    auto pos = cj["position"];
                    c->position = glm::vec3(pos[0].get<float>(), pos[1].get<float>(), pos[2].get<float>());
                }
                if (cj.contains("health")) c->stats.health = cj["health"].get<float>();
                if (cj.contains("stamina")) c->stats.stamina = cj["stamina"].get<float>();

                if (auto* boyd = dynamic_cast<Boyd*>(c)) {
                    if (cj.contains("curseMeter")) boyd->curseMeter = cj["curseMeter"].get<float>();
                    if (cj.contains("revolverAmmo")) boyd->revolverAmmo = cj["revolverAmmo"].get<int>();
                    if (cj.contains("reserveAmmo")) boyd->reserveAmmo = cj["reserveAmmo"].get<int>();
                } else if (auto* jade = dynamic_cast<Jade*>(c)) {
                    if (cj.contains("visionTimer")) jade->visionTimer = cj["visionTimer"].get<float>();
                    if (cj.contains("decodedGlyphs")) {
                        jade->decodedGlyphs.clear();
                        for (auto g : cj["decodedGlyphs"]) {
                            jade->decodedGlyphs.push_back(g.get<int>());
                        }
                    }
                } else if (auto* tabitha = dynamic_cast<Tabitha*>(c)) {
                    if (cj.contains("revealedTunnelSections")) {
                        tabitha->revealedTunnelSections.clear();
                        for (auto s : cj["revealedTunnelSections"]) {
                            tabitha->revealedTunnelSections.push_back(s.get<int>());
                        }
                    }
                } else if (auto* victor = dynamic_cast<Victor*>(c)) {
                    if (cj.contains("talismansCarried")) victor->talismansCarried = cj["talismansCarried"].get<int>();
                    if (cj.contains("elderTalismansCarried")) victor->elderTalismansCarried = cj["elderTalismansCarried"].get<int>();
                } else if (auto* sara = dynamic_cast<Sara*>(c)) {
                    if (cj.contains("redemptionScore")) sara->redemptionScore = cj["redemptionScore"].get<float>();
                }
            }
        }
    }

    if (j.contains("npcs")) {
        // Simple mapping based on id if available, assuming list exists.
        auto npcsJson = j["npcs"];
        for (auto& nj : npcsJson) {
            int id = nj["id"].get<int>();
            for (NPC* npc : npcs) {
                if (npc->id == id) {
                    if (nj.contains("health")) npc->health = nj["health"].get<float>();
                    if (nj.contains("alive")) npc->alive = nj["alive"].get<bool>();
                    if (nj.contains("position")) {
                        auto pos = nj["position"];
                        npc->position = glm::vec3(pos[0].get<float>(), pos[1].get<float>(), pos[2].get<float>());
                    }
                    break;
                }
            }
        }
    }

    if (j.contains("activeCharacter")) {
        cm.SetActiveCharacter(j["activeCharacter"].get<std::string>());
    }

    return true;
}
