#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>

enum class QuestState {
    NotStarted,
    InProgress,
    PartialComplete,
    Complete,
    Failed
};

enum class QuestType {
    Boyd_SolveConspiracy,
    Jade_DecodeSymbols,
    Tabitha_MapTunnels,
    Victor_RecallPast,
    Sara_FindRedemption
};

// Individual quest objective/step
struct QuestObjective {
    std::string description;
    bool completed = false;
    float progressPercent = 0.0f;  // 0-100
};

// Main quest class - each character has one active quest
class Quest {
public:
    explicit Quest(QuestType type, std::string title);

    QuestType GetType() const { return type; }
    const std::string& GetTitle() const { return title; }
    QuestState GetState() const { return state; }
    
    void Start();
    void AdvanceProgress(float amount);  // Add progress (0-1 per call)
    void CompleteObjective(int index);
    bool IsComplete() const { return state == QuestState::Complete; }
    bool IsFailed() const { return state == QuestState::Failed; }
    
    float GetProgress() const { return progressPercent; }
    const std::vector<QuestObjective>& GetObjectives() const { return objectives; }
    
    // Consequence: other characters get affected
    const std::string& GetConsequenceDescription() const { return consequenceDesc; }
    bool HasConsequence() const { return !consequenceDesc.empty(); }

private:
    QuestType type;
    std::string title;
    QuestState state = QuestState::NotStarted;
    float progressPercent = 0.0f;  // 0-100
    std::vector<QuestObjective> objectives;
    std::string consequenceDesc;  // What happens to other characters
    
    void InitializeObjectives();  // Called in constructor to set up quest steps
};
