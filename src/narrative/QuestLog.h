#pragma once

#include <functional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

// ---------------------------------------------------------------------------
// QuestObjective — a single completable step within a quest.
// completionCheck is a lambda/functor queried each frame by QuestLog::Update.
// ---------------------------------------------------------------------------
struct QuestObjective {
    std::string description;
    std::function<bool()> completionCheck;  // may be nullptr (manual complete)
    bool completed = false;
};

// ---------------------------------------------------------------------------
// Quest — top-level quest entry with multiple objectives.
// ---------------------------------------------------------------------------
struct Quest {
    std::string id;
    std::string title;
    std::string description;
    std::string assignedCharacter;  // e.g. "Sara", "Boyd" — can be empty for shared

    std::vector<QuestObjective> objectives;

    bool completed = false;
    bool failed    = false;

    /// NarrativeEngine flag to set when the quest completes.
    std::string onCompleteFlag;
};

// ---------------------------------------------------------------------------
// QuestLog — owns all quests; polls objectives every frame.
// ---------------------------------------------------------------------------
class QuestLog {
public:
    /// Add a new quest. Duplicate IDs are ignored.
    void AddQuest(Quest q);

    /// Poll each uncompleted objective's completionCheck and advance state.
    /// Fires EventBus::QUEST_UPDATED / QUEST_COMPLETED as needed.
    void Update();

    /// Returns nullptr if not found.
    Quest* GetQuest(const std::string& id);
    const Quest* GetQuest(const std::string& id) const;

    /// Active = not completed AND not failed.
    std::vector<Quest*> GetActiveQuests(const std::string& characterName = "");
    std::vector<Quest*> GetCompletedQuests(const std::string& characterName = "");

    /// Serialize / deserialize (objectives & completionCheck excluded from JSON).
    nlohmann::json Serialize() const;
    void           Deserialize(const nlohmann::json& j);

private:
    std::vector<Quest> quests_;

    void AdvanceQuest(Quest& q);
};
