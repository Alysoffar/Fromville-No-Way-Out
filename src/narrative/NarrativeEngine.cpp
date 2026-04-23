#include "NarrativeEngine.h"

#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <variant>

#include <nlohmann/json.hpp>

#include "QuestLog.h"
#include "characters/Boyd.h"
#include "characters/Character.h"
#include "characters/Sara.h"
#include "characters/Victor.h"
#include "core/EventBus.h"

// ---------------------------------------------------------------------------
// Payload helper
// ---------------------------------------------------------------------------
namespace {

template<typename T>
T PayloadGet(const PayloadMap& payload, const std::string& key, T def = T{}) {
    auto it = payload.find(key);
    if (it == payload.end()) {
        return def;
    }
    if (const T* val = std::get_if<T>(&it->second)) {
        return *val;
    }
    return def;
}

} // namespace

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

NarrativeEngine::NarrativeEngine(QuestLog* quests, EventBus* bus)
    : questLog_(quests), bus_(bus) {}

// ---------------------------------------------------------------------------
// Init — subscribe to all relevant events
// ---------------------------------------------------------------------------

void NarrativeEngine::Init(std::vector<Character*>& characters) {
    characters_ = characters;

    auto sub = [this](GameEvent ev, auto fn) {
        subscriptionIds_.push_back(
            bus_->Subscribe(ev, [this, fn](const EventData& e) { (this->*fn)(e); }));
    };

    sub(GameEvent::NPC_DIED,                   &NarrativeEngine::OnNPCDied);
    sub(GameEvent::GLYPH_DECODED,              &NarrativeEngine::OnGlyphDecoded);
    sub(GameEvent::TALISMAN_PLACED,            &NarrativeEngine::OnTalismanPlaced);
    sub(GameEvent::NPC_OPENED_DOOR,            &NarrativeEngine::OnNPCOpenedDoor);
    sub(GameEvent::BOYD_RAGE_START,            &NarrativeEngine::OnBoydRageStart);
    sub(GameEvent::SARA_REDEMPTION_CHANGED,    &NarrativeEngine::OnSaraRedemptionChanged);
    sub(GameEvent::DAWN_BEGAN,                 &NarrativeEngine::OnDawnBegan);
}

// ---------------------------------------------------------------------------
// Per-frame processing — flush the EventBus deferred queue
// ---------------------------------------------------------------------------

void NarrativeEngine::ProcessEvents() {
    bus_->ProcessQueued();
    if (questLog_) {
        questLog_->Update();
    }
}

// ---------------------------------------------------------------------------
// Flag API
// ---------------------------------------------------------------------------

void NarrativeEngine::SetFlag(const std::string& flag, bool value) {
    flags_[flag] = value;
    std::printf("NarrativeEngine: flag '%s' = %s\n",
                flag.c_str(), value ? "true" : "false");
}

bool NarrativeEngine::GetFlag(const std::string& flag) const {
    auto it = flags_.find(flag);
    return it != flags_.end() && it->second;
}

const std::unordered_map<std::string, bool>& NarrativeEngine::GetAllFlags() const {
    return flags_;
}

// ---------------------------------------------------------------------------
// JSON quest loading
// ---------------------------------------------------------------------------

void NarrativeEngine::LoadQuestsFromJSON(const std::string& path) {
    if (!questLog_) {
        return;
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        std::printf("NarrativeEngine: cannot open quest file '%s'\n", path.c_str());
        return;
    }

    nlohmann::json j;
    try {
        file >> j;
    } catch (const std::exception& e) {
        std::printf("NarrativeEngine: JSON parse error in '%s': %s\n",
                    path.c_str(), e.what());
        return;
    }

    if (!j.is_array()) {
        std::printf("NarrativeEngine: expected JSON array in '%s'\n", path.c_str());
        return;
    }

    for (const auto& qj : j) {
        Quest q;
        q.id                = qj.value("id", "");
        q.title             = qj.value("title", "");
        q.description       = qj.value("description", "");
        q.assignedCharacter = qj.value("assignedCharacter", "");
        q.onCompleteFlag    = qj.value("onCompleteFlag", "");

        if (qj.contains("objectives") && qj["objectives"].is_array()) {
            for (const auto& oj : qj["objectives"]) {
                QuestObjective obj;
                obj.description = oj.value("description", "");
                // completionCheck must be wired at runtime by game code
                q.objectives.push_back(std::move(obj));
            }
        }

        if (!q.id.empty()) {
            questLog_->AddQuest(std::move(q));
        }
    }
    std::printf("NarrativeEngine: loaded quests from '%s'\n", path.c_str());
}

// ---------------------------------------------------------------------------
// Character lookups
// ---------------------------------------------------------------------------

Sara* NarrativeEngine::GetSara() const {
    for (Character* c : characters_) {
        if (auto* s = dynamic_cast<Sara*>(c)) {
            return s;
        }
    }
    return nullptr;
}

Boyd* NarrativeEngine::GetBoyd() const {
    for (Character* c : characters_) {
        if (auto* b = dynamic_cast<Boyd*>(c)) {
            return b;
        }
    }
    return nullptr;
}

Victor* NarrativeEngine::GetVictor() const {
    for (Character* c : characters_) {
        if (auto* v = dynamic_cast<Victor*>(c)) {
            return v;
        }
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// Consequence: NPC_DIED
//   payload keys: "npc_name" (string)
// ---------------------------------------------------------------------------

void NarrativeEngine::OnNPCDied(const EventData& e) {
    const std::string npcName = PayloadGet<std::string>(e.payload, "npc_name");

    if (npcName == "Kenny") {
        if (Sara* sara = GetSara()) {
            sara->AddRedemptionScore(-25.0f);
        }
        SetFlag("kenny_dead", true);

        // Victor loses a memory fragment (decrement talismansCarried as proxy)
        if (Victor* victor = GetVictor()) {
            victor->talismansCarried = std::max(0, victor->talismansCarried - 1);
            std::printf("NarrativeEngine: Victor lost a memory fragment (Kenny died)\n");
        }
    } else if (npcName == "Donna") {
        SetFlag("colony_house_leaderless", true);
    }

    // Fire QUEST_UPDATED for any active quest that references this NPC
    if (questLog_) {
        for (Quest* q : questLog_->GetActiveQuests()) {
            if (q->assignedCharacter == npcName ||
                q->description.find(npcName) != std::string::npos) {
                bus_->Fire(GameEvent::QUEST_UPDATED,
                           {{"quest_id", q->id}, {"reason", std::string("npc_died")}, {"npc_name", npcName}});
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Consequence: GLYPH_DECODED
//   payload keys: "glyph_id" (int)
// ---------------------------------------------------------------------------

void NarrativeEngine::OnGlyphDecoded(const EventData& e) {
    const int glyphId = PayloadGet<int>(e.payload, "glyph_id", -1);
    if (glyphId < 0) {
        return;
    }

    SetFlag("glyph_decoded_" + std::to_string(glyphId), true);

    // Check if all three main glyphs are decoded
    if (GetFlag("glyph_decoded_1") &&
        GetFlag("glyph_decoded_2") &&
        GetFlag("glyph_decoded_3")) {

        if (!GetFlag("jade_main_arc_complete")) {
            SetFlag("jade_main_arc_complete", true);
            bus_->Fire(GameEvent::QUEST_COMPLETED,
                       {{"quest_id", std::string("jade_glyph_quest")}});
            std::printf("NarrativeEngine: Jade's main glyph arc complete!\n");
        }
    }
}

// ---------------------------------------------------------------------------
// Consequence: TALISMAN_PLACED
//   payload keys: "character" (string), "talisman_type" (string)
// ---------------------------------------------------------------------------

void NarrativeEngine::OnTalismanPlaced(const EventData& e) {
    const std::string character    = PayloadGet<std::string>(e.payload, "character");
    const std::string talismanType = PayloadGet<std::string>(e.payload, "talisman_type");

    if (!character.empty()) {
        SetFlag("talisman_placed_by_" + character, true);
    }
    if (talismanType == "elder") {
        SetFlag("elder_talisman_active", true);
        std::printf("NarrativeEngine: elder talisman placed by %s\n", character.c_str());
    }
}

// ---------------------------------------------------------------------------
// Consequence: NPC_OPENED_DOOR
//   payload keys: "npc_name" (string), "door_id" (string)
// ---------------------------------------------------------------------------

void NarrativeEngine::OnNPCOpenedDoor(const EventData& e) {
    const std::string npcName = PayloadGet<std::string>(e.payload, "npc_name");
    const std::string doorId  = PayloadGet<std::string>(e.payload, "door_id");

    if (!npcName.empty() && !doorId.empty()) {
        SetFlag("door_" + doorId + "_opened_by_" + npcName, true);
        std::printf("NarrativeEngine: %s opened door '%s'\n",
                    npcName.c_str(), doorId.c_str());
    }
}

// ---------------------------------------------------------------------------
// Consequence: BOYD_RAGE_START
// ---------------------------------------------------------------------------

void NarrativeEngine::OnBoydRageStart(const EventData& /*e*/) {
    SetFlag("boyd_raged_at_least_once", true);
    // Witnessed NPCs will fear Boyd — reflected in dialogue via flag checks.
    // Any NPC dialogue tree can gate lines on "boyd_raged_at_least_once".
    std::printf("NarrativeEngine: Boyd raged — NPCs will remember this.\n");
}

// ---------------------------------------------------------------------------
// Consequence: SARA_REDEMPTION_CHANGED
//   payload keys: "score" (float)
// ---------------------------------------------------------------------------

void NarrativeEngine::OnSaraRedemptionChanged(const EventData& e) {
    const float score = PayloadGet<float>(e.payload, "score", 0.0f);

    if (score >= 50.0f && !GetFlag("sara_half_redeemed")) {
        SetFlag("sara_half_redeemed", true);
        // Unlock a Victor dialogue acknowledging Sara's change.
        SetFlag("victor_can_comment_sara_change", true);
        std::printf("NarrativeEngine: Sara half-redeemed (%.1f) — Victor dialogue unlocked.\n",
                    score);
    }

    if (score >= 80.0f && !GetFlag("sara_fully_redeemed")) {
        SetFlag("sara_fully_redeemed", true);
        bus_->Fire(GameEvent::QUEST_COMPLETED,
                   {{"quest_id", std::string("sara_redemption_arc")}});
        std::printf("NarrativeEngine: Sara fully redeemed (%.1f)!\n", score);
    }
}

// ---------------------------------------------------------------------------
// Consequence: DAWN_BEGAN
// ---------------------------------------------------------------------------

void NarrativeEngine::OnDawnBegan(const EventData& /*e*/) {
    SetFlag("is_daytime", true);
    SetFlag("is_nighttime", false);
    std::printf("NarrativeEngine: Dawn — day/night flags updated.\n");

    // Any night-only events can be flagged as expired here.
    if (GetFlag("elder_talisman_active")) {
        // Elder talismans expire at dawn.
        SetFlag("elder_talisman_active", false);
        bus_->Fire(GameEvent::ELDER_TALISMAN_EXPIRED, {});
    }
}
