// narrative_test.cpp — standalone smoke-test for EventBus, QuestLog, NarrativeEngine
// Compile: g++ -std=c++20 -I../../include -I.. narrative_test.cpp
//          ../narrative/QuestLog.cpp ../narrative/NarrativeEngine.cpp
//          ../core/EventBus.cpp
//          ../characters/Sara.cpp ../characters/Boyd.cpp ../characters/Victor.cpp
//          ../characters/Character.cpp ../characters/Jade.cpp ../characters/Tabitha.cpp
//          ../renderer/SkeletalAnimator.cpp ../renderer/Model.cpp ../renderer/Mesh.cpp
//          ... (link against openal, sndfile, assimp, glfw, GL, Freetype, nlohmann_json)
//
// For a quick build-test, we just compile within the existing CMake project
// by adding this file to the source list, which is not what we want. Instead
// we use main() and guard it behind NARRATIVE_TEST_MAIN so it replaces
// the real main only when the test is built separately.
//
// Since we cannot easily run a headless standalone here, we embed the tests
// directly into the CMake build via a test runner added to main.cpp temporarily.
// This file is compiled as part of the existing project only when FROMVILLE_TESTS=1.

// ----- TEST RUNNER (included from main.cpp guard) -----
#ifndef NARRATIVE_TEST_MAIN
#define NARRATIVE_TEST_MAIN
#endif

#include <cassert>
#include <cstdio>
#include <string>
#include <vector>

// Bring in subsystems we are testing
#include "core/EventBus.h"
#include "narrative/QuestLog.h"
#include "narrative/NarrativeEngine.h"

// We need a minimal Character stub so NarrativeEngine character lookups work.
// The real headers are included transitively through NarrativeEngine.h.
#include "characters/Sara.h"
#include "characters/Boyd.h"
#include "characters/Victor.h"

// ──────────────────────────────────────────────────────────────────────────────
// Helpers
// ──────────────────────────────────────────────────────────────────────────────
static int  gPassed = 0;
static int  gFailed = 0;

#define CHECK(cond) \
    do { \
        if (cond) { \
            std::printf("  [PASS] %s\n", #cond); \
            ++gPassed; \
        } else { \
            std::printf("  [FAIL] %s  (line %d)\n", #cond, __LINE__); \
            ++gFailed; \
        } \
    } while(false)

// ──────────────────────────────────────────────────────────────────────────────
// Test: EventBus — subscribe / fire / unsubscribe
// ──────────────────────────────────────────────────────────────────────────────
static void Test_EventBus_Basic() {
    std::printf("\n--- EventBus: Subscribe / Fire / Unsubscribe ---\n");
    EventBus& bus = EventBus::Get();

    int counter = 0;
    int subId = bus.Subscribe(GameEvent::PLAYER_DAMAGED, [&](const EventData&) {
        ++counter;
    });

    bus.Fire(GameEvent::PLAYER_DAMAGED);
    CHECK(counter == 1);

    bus.Fire(GameEvent::PLAYER_DAMAGED);
    CHECK(counter == 2);

    bus.Unsubscribe(subId);
    bus.Fire(GameEvent::PLAYER_DAMAGED);
    CHECK(counter == 2);  // should not increment after unsub
}

// ──────────────────────────────────────────────────────────────────────────────
// Test: EventBus — payload extraction
// ──────────────────────────────────────────────────────────────────────────────
static void Test_EventBus_Payload() {
    std::printf("\n--- EventBus: Payload round-trip ---\n");
    EventBus& bus = EventBus::Get();

    std::string receivedNpc;
    float receivedScore = -1.0f;

    int subId = bus.Subscribe(GameEvent::SARA_REDEMPTION_CHANGED,
        [&](const EventData& e) {
            auto it = e.payload.find("npc");
            if (it != e.payload.end()) {
                if (const std::string* s = std::get_if<std::string>(&it->second)) {
                    receivedNpc = *s;
                }
            }
            auto it2 = e.payload.find("score");
            if (it2 != e.payload.end()) {
                if (const float* f = std::get_if<float>(&it2->second)) {
                    receivedScore = *f;
                }
            }
        });

    bus.Fire(GameEvent::SARA_REDEMPTION_CHANGED,
             {{"npc", std::string("Sara")}, {"score", 55.0f}});

    CHECK(receivedNpc == "Sara");
    CHECK(receivedScore > 54.9f && receivedScore < 55.1f);

    bus.Unsubscribe(subId);
}

// ──────────────────────────────────────────────────────────────────────────────
// Test: EventBus — deferred queue
// ──────────────────────────────────────────────────────────────────────────────
static void Test_EventBus_Deferred() {
    std::printf("\n--- EventBus: Defer / ProcessQueued ---\n");
    EventBus& bus = EventBus::Get();

    int counter = 0;
    int subId = bus.Subscribe(GameEvent::QUEST_UPDATED, [&](const EventData&) {
        ++counter;
    });

    bus.Defer(GameEvent::QUEST_UPDATED, {{"quest_id", std::string("test_q")}});
    CHECK(counter == 0);   // not fired yet

    bus.ProcessQueued();
    CHECK(counter == 1);   // fired now

    bus.ProcessQueued();
    CHECK(counter == 1);   // queue empty, no double-fire

    bus.Unsubscribe(subId);
}

// ──────────────────────────────────────────────────────────────────────────────
// Test: QuestLog — add, complete objectives, events
// ──────────────────────────────────────────────────────────────────────────────
static void Test_QuestLog_Objectives() {
    std::printf("\n--- QuestLog: Objective completion ---\n");
    EventBus& bus = EventBus::Get();

    bool questCompletedFired = false;
    int subId = bus.Subscribe(GameEvent::QUEST_COMPLETED, [&](const EventData& e) {
        auto it = e.payload.find("quest_id");
        if (it != e.payload.end()) {
            if (const std::string* id = std::get_if<std::string>(&it->second)) {
                if (*id == "test_quest") {
                    questCompletedFired = true;
                }
            }
        }
    });

    bool condition1 = false;
    bool condition2 = false;

    Quest q;
    q.id    = "test_quest";
    q.title = "Test Quest";

    QuestObjective obj1;
    obj1.description     = "Do thing 1";
    obj1.completionCheck = [&]{ return condition1; };

    QuestObjective obj2;
    obj2.description     = "Do thing 2";
    obj2.completionCheck = [&]{ return condition2; };

    q.objectives.push_back(std::move(obj1));
    q.objectives.push_back(std::move(obj2));

    QuestLog log;
    log.AddQuest(std::move(q));

    log.Update();
    CHECK(log.GetQuest("test_quest") != nullptr);
    CHECK(!log.GetQuest("test_quest")->completed);
    CHECK(!questCompletedFired);

    condition1 = true;
    log.Update();
    CHECK(!log.GetQuest("test_quest")->completed);  // only 1 of 2 done

    condition2 = true;
    log.Update();
    CHECK(log.GetQuest("test_quest")->completed);
    CHECK(questCompletedFired);

    bus.Unsubscribe(subId);
}

// ──────────────────────────────────────────────────────────────────────────────
// Test: QuestLog — GetActiveQuests / GetCompletedQuests
// ──────────────────────────────────────────────────────────────────────────────
static void Test_QuestLog_Filters() {
    std::printf("\n--- QuestLog: Active / Completed filters ---\n");

    QuestLog log;
    {
        Quest q;
        q.id                = "sara_q";
        q.assignedCharacter = "Sara";
        QuestObjective o; o.description = "x"; o.completionCheck = []{ return false; };
        q.objectives.push_back(o);
        log.AddQuest(std::move(q));
    }
    {
        Quest q;
        q.id                = "boyd_q";
        q.assignedCharacter = "Boyd";
        QuestObjective o; o.description = "y"; o.completionCheck = []{ return true; };
        q.objectives.push_back(o);
        log.AddQuest(std::move(q));
    }

    log.Update();  // boyd_q completes

    auto allActive = log.GetActiveQuests();
    auto saraActive = log.GetActiveQuests("Sara");
    auto completed  = log.GetCompletedQuests();

    CHECK(allActive.size() == 1);
    CHECK(saraActive.size() == 1);
    CHECK(completed.size() == 1);
    CHECK(completed[0]->id == "boyd_q");
}

// ──────────────────────────────────────────────────────────────────────────────
// Test: QuestLog — Serialize / Deserialize
// ──────────────────────────────────────────────────────────────────────────────
static void Test_QuestLog_Serialization() {
    std::printf("\n--- QuestLog: JSON Serialize / Deserialize ---\n");

    QuestLog log;
    {
        Quest q;
        q.id          = "serial_q";
        q.title       = "Serialized Quest";
        q.description = "A test quest";
        q.assignedCharacter = "Victor";
        QuestObjective o; o.description = "Step 1"; o.completed = true;
        q.objectives.push_back(o);
        log.AddQuest(std::move(q));
    }

    nlohmann::json j = log.Serialize();
    CHECK(!j.empty());
    CHECK(j[0]["id"] == "serial_q");
    CHECK(j[0]["objectives"][0]["completed"] == true);

    QuestLog log2;
    log2.Deserialize(j);
    CHECK(log2.GetQuest("serial_q") != nullptr);
    CHECK(log2.GetQuest("serial_q")->title == "Serialized Quest");
    CHECK(!log2.GetQuest("serial_q")->objectives.empty());
    CHECK(log2.GetQuest("serial_q")->objectives[0].completed == true);
}

// ──────────────────────────────────────────────────────────────────────────────
// Test: NarrativeEngine — flags
// ──────────────────────────────────────────────────────────────────────────────
static void Test_NarrativeEngine_Flags() {
    std::printf("\n--- NarrativeEngine: SetFlag / GetFlag ---\n");

    QuestLog log;
    EventBus& bus = EventBus::Get();
    NarrativeEngine engine(&log, &bus);
    std::vector<Character*> chars;
    engine.Init(chars);

    CHECK(!engine.GetFlag("some_flag"));
    engine.SetFlag("some_flag");
    CHECK(engine.GetFlag("some_flag"));
    engine.SetFlag("some_flag", false);
    CHECK(!engine.GetFlag("some_flag"));
}

// ──────────────────────────────────────────────────────────────────────────────
// Test: NarrativeEngine — OnGlyphDecoded consequence
// ──────────────────────────────────────────────────────────────────────────────
static void Test_NarrativeEngine_GlyphArc() {
    std::printf("\n--- NarrativeEngine: Glyph arc completion ---\n");

    QuestLog log;
    {
        Quest q;
        q.id = "jade_glyph_quest";
        q.title = "Jade's Glyph Quest";
        QuestObjective o;
        o.description = "Decode all glyphs";
        o.completionCheck = nullptr;  // driven by NarrativeEngine flags
        q.objectives.push_back(o);
        log.AddQuest(std::move(q));
    }

    EventBus& bus = EventBus::Get();
    bool questCompletedFired = false;
    int subId = bus.Subscribe(GameEvent::QUEST_COMPLETED, [&](const EventData& e) {
        auto it = e.payload.find("quest_id");
        if (it != e.payload.end()) {
            if (const std::string* id = std::get_if<std::string>(&it->second)) {
                if (*id == "jade_glyph_quest") questCompletedFired = true;
            }
        }
    });

    NarrativeEngine engine(&log, &bus);
    std::vector<Character*> chars;
    engine.Init(chars);

    bus.Fire(GameEvent::GLYPH_DECODED, {{"glyph_id", 1}});
    CHECK(engine.GetFlag("glyph_decoded_1"));
    CHECK(!engine.GetFlag("jade_main_arc_complete"));
    CHECK(!questCompletedFired);

    bus.Fire(GameEvent::GLYPH_DECODED, {{"glyph_id", 2}});
    bus.Fire(GameEvent::GLYPH_DECODED, {{"glyph_id", 3}});
    CHECK(engine.GetFlag("glyph_decoded_2"));
    CHECK(engine.GetFlag("glyph_decoded_3"));
    CHECK(engine.GetFlag("jade_main_arc_complete"));
    CHECK(questCompletedFired);

    bus.Unsubscribe(subId);
}

// ──────────────────────────────────────────────────────────────────────────────
// Test: NarrativeEngine — OnNPCDied (Kenny)
// ──────────────────────────────────────────────────────────────────────────────
static void Test_NarrativeEngine_NPCDied() {
    std::printf("\n--- NarrativeEngine: NPC_DIED (Kenny) ---\n");

    QuestLog log;
    EventBus& bus = EventBus::Get();
    NarrativeEngine engine(&log, &bus);

    Sara sara;
    Victor victor;
    victor.talismansCarried = 3;
    std::vector<Character*> chars = { &sara, &victor };
    engine.Init(chars);

    const float redemptionBefore = sara.redemptionScore;
    bus.Fire(GameEvent::NPC_DIED, {{"npc_name", std::string("Kenny")}});

    CHECK(sara.redemptionScore == redemptionBefore - 25.0f);
    CHECK(engine.GetFlag("kenny_dead"));
    CHECK(victor.talismansCarried == 2);  // lost one fragment
}

// ──────────────────────────────────────────────────────────────────────────────
// Test: NarrativeEngine — OnSaraRedemptionChanged thresholds
// ──────────────────────────────────────────────────────────────────────────────
static void Test_NarrativeEngine_SaraRedemption() {
    std::printf("\n--- NarrativeEngine: Sara redemption thresholds ---\n");

    QuestLog log;
    {
        Quest q;
        q.id = "sara_redemption_arc";
        q.title = "Sara's Redemption";
        QuestObjective o; o.description = "Redeem Sara"; o.completionCheck = nullptr;
        q.objectives.push_back(o);
        log.AddQuest(std::move(q));
    }

    EventBus& bus = EventBus::Get();
    bool arcFired = false;
    int subId = bus.Subscribe(GameEvent::QUEST_COMPLETED, [&](const EventData& e) {
        auto it = e.payload.find("quest_id");
        if (it != e.payload.end()) {
            if (const std::string* id = std::get_if<std::string>(&it->second)) {
                if (*id == "sara_redemption_arc") arcFired = true;
            }
        }
    });

    NarrativeEngine engine(&log, &bus);
    std::vector<Character*> chars;
    engine.Init(chars);

    bus.Fire(GameEvent::SARA_REDEMPTION_CHANGED, {{"score", 30.0f}});
    CHECK(!engine.GetFlag("sara_half_redeemed"));
    CHECK(!engine.GetFlag("sara_fully_redeemed"));

    bus.Fire(GameEvent::SARA_REDEMPTION_CHANGED, {{"score", 55.0f}});
    CHECK(engine.GetFlag("sara_half_redeemed"));
    CHECK(engine.GetFlag("victor_can_comment_sara_change"));
    CHECK(!engine.GetFlag("sara_fully_redeemed"));

    bus.Fire(GameEvent::SARA_REDEMPTION_CHANGED, {{"score", 85.0f}});
    CHECK(engine.GetFlag("sara_fully_redeemed"));
    CHECK(arcFired);

    bus.Unsubscribe(subId);
}

// ──────────────────────────────────────────────────────────────────────────────
// Test: NarrativeEngine — OnBoydRageStart + OnDawnBegan
// ──────────────────────────────────────────────────────────────────────────────
static void Test_NarrativeEngine_Misc() {
    std::printf("\n--- NarrativeEngine: Boyd rage + Dawn ---\n");

    QuestLog log;
    EventBus& bus = EventBus::Get();
    NarrativeEngine engine(&log, &bus);
    std::vector<Character*> chars;
    engine.Init(chars);

    bus.Fire(GameEvent::BOYD_RAGE_START);
    CHECK(engine.GetFlag("boyd_raged_at_least_once"));

    // Simulate elder talisman active before dawn
    engine.SetFlag("elder_talisman_active", true);

    bool elderExpiredFired = false;
    int subId = bus.Subscribe(GameEvent::ELDER_TALISMAN_EXPIRED, [&](const EventData&) {
        elderExpiredFired = true;
    });

    bus.Fire(GameEvent::DAWN_BEGAN);
    CHECK(engine.GetFlag("is_daytime"));
    CHECK(!engine.GetFlag("is_nighttime"));
    CHECK(!engine.GetFlag("elder_talisman_active"));
    CHECK(elderExpiredFired);

    bus.Unsubscribe(subId);
}

// ──────────────────────────────────────────────────────────────────────────────
// Entry point
// ──────────────────────────────────────────────────────────────────────────────
int RunNarrativeTests() {
    std::printf("╔══════════════════════════════════════════════════╗\n");
    std::printf("║      Fromville Narrative System Test Suite       ║\n");
    std::printf("╚══════════════════════════════════════════════════╝\n");

    Test_EventBus_Basic();
    Test_EventBus_Payload();
    Test_EventBus_Deferred();
    Test_QuestLog_Objectives();
    Test_QuestLog_Filters();
    Test_QuestLog_Serialization();
    Test_NarrativeEngine_Flags();
    Test_NarrativeEngine_GlyphArc();
    Test_NarrativeEngine_NPCDied();
    Test_NarrativeEngine_SaraRedemption();
    Test_NarrativeEngine_Misc();

    std::printf("\n══════════════════════════════════════════════════\n");
    std::printf("Results: %d passed, %d failed\n", gPassed, gFailed);
    std::printf("══════════════════════════════════════════════════\n\n");

    return (gFailed == 0) ? 0 : 1;
}
