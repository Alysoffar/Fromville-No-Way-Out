#include "QuestLog.h"

#include <algorithm>
#include <cstdio>

#include "core/EventBus.h"

// ---------------------------------------------------------------------------
// AddQuest
// ---------------------------------------------------------------------------

void QuestLog::AddQuest(Quest q) {
    for (const Quest& existing : quests_) {
        if (existing.id == q.id) {
            std::printf("QuestLog: quest '%s' already exists — skipping\n",
                        q.id.c_str());
            return;
        }
    }
    quests_.push_back(std::move(q));
}

// ---------------------------------------------------------------------------
// Update — advance every active quest
// ---------------------------------------------------------------------------

void QuestLog::Update() {
    for (Quest& q : quests_) {
        if (q.completed || q.failed) {
            continue;
        }
        AdvanceQuest(q);
    }
}

void QuestLog::AdvanceQuest(Quest& q) {
    bool anyChanged = false;

    for (QuestObjective& obj : q.objectives) {
        if (obj.completed) {
            continue;
        }
        if (obj.completionCheck && obj.completionCheck()) {
            obj.completed = true;
            anyChanged    = true;
            std::printf("QuestLog: objective complete in '%s': %s\n",
                        q.id.c_str(), obj.description.c_str());
        }
    }

    if (anyChanged) {
        EventBus::Get().Fire(GameEvent::QUEST_UPDATED,
                             {{"quest_id", q.id}});
    }

    // Check if all objectives are done
    const bool allDone = !q.objectives.empty() &&
        std::all_of(q.objectives.begin(), q.objectives.end(),
                    [](const QuestObjective& o) { return o.completed; });

    if (allDone && !q.completed) {
        q.completed = true;
        std::printf("QuestLog: quest '%s' completed!\n", q.id.c_str());

        PayloadMap payload;
        payload["quest_id"] = q.id;
        if (!q.onCompleteFlag.empty()) {
            payload["on_complete_flag"] = q.onCompleteFlag;
        }
        EventBus::Get().Fire(GameEvent::QUEST_COMPLETED, std::move(payload));
    }
}

// ---------------------------------------------------------------------------
// Accessors
// ---------------------------------------------------------------------------

Quest* QuestLog::GetQuest(const std::string& id) {
    for (Quest& q : quests_) {
        if (q.id == id) {
            return &q;
        }
    }
    return nullptr;
}

const Quest* QuestLog::GetQuest(const std::string& id) const {
    for (const Quest& q : quests_) {
        if (q.id == id) {
            return &q;
        }
    }
    return nullptr;
}

std::vector<Quest*> QuestLog::GetActiveQuests(const std::string& characterName) {
    std::vector<Quest*> result;
    for (Quest& q : quests_) {
        if (q.completed || q.failed) {
            continue;
        }
        if (!characterName.empty() && q.assignedCharacter != characterName) {
            continue;
        }
        result.push_back(&q);
    }
    return result;
}

std::vector<Quest*> QuestLog::GetCompletedQuests(const std::string& characterName) {
    std::vector<Quest*> result;
    for (Quest& q : quests_) {
        if (!q.completed) {
            continue;
        }
        if (!characterName.empty() && q.assignedCharacter != characterName) {
            continue;
        }
        result.push_back(&q);
    }
    return result;
}

// ---------------------------------------------------------------------------
// Serialization
// Note: completionCheck lambdas cannot be serialized; they must be
// re-registered after Deserialize via AddQuest or quest rebuild.
// ---------------------------------------------------------------------------

nlohmann::json QuestLog::Serialize() const {
    nlohmann::json j = nlohmann::json::array();
    for (const Quest& q : quests_) {
        nlohmann::json qj;
        qj["id"]                = q.id;
        qj["title"]             = q.title;
        qj["description"]       = q.description;
        qj["assignedCharacter"] = q.assignedCharacter;
        qj["completed"]         = q.completed;
        qj["failed"]            = q.failed;
        qj["onCompleteFlag"]    = q.onCompleteFlag;

        nlohmann::json objArray = nlohmann::json::array();
        for (const QuestObjective& obj : q.objectives) {
            nlohmann::json oj;
            oj["description"] = obj.description;
            oj["completed"]   = obj.completed;
            objArray.push_back(oj);
        }
        qj["objectives"] = objArray;
        j.push_back(qj);
    }
    return j;
}

void QuestLog::Deserialize(const nlohmann::json& j) {
    quests_.clear();
    if (!j.is_array()) {
        return;
    }
    for (const auto& qj : j) {
        Quest q;
        q.id                = qj.value("id", "");
        q.title             = qj.value("title", "");
        q.description       = qj.value("description", "");
        q.assignedCharacter = qj.value("assignedCharacter", "");
        q.completed         = qj.value("completed", false);
        q.failed            = qj.value("failed", false);
        q.onCompleteFlag    = qj.value("onCompleteFlag", "");

        if (qj.contains("objectives") && qj["objectives"].is_array()) {
            for (const auto& oj : qj["objectives"]) {
                QuestObjective obj;
                obj.description = oj.value("description", "");
                obj.completed   = oj.value("completed", false);
                // completionCheck must be re-wired by the caller
                q.objectives.push_back(std::move(obj));
            }
        }
        quests_.push_back(std::move(q));
    }
}
