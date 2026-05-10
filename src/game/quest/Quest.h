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

// Objective types for varied gameplay
enum class ObjectiveType {
    Dialogue,       // Talk to NPC and get information
    Collect,        // Gather items/evidence
    Puzzle,         // Solve a puzzle/challenge
    Observe,        // Sneak/observe without being detected
    Combat,         // Defeat enemies
    Timed,          // Complete within time limit
    Skill,          // Requires specific character type
    Environmental   // Find clues in environment
};

// Sub-objective for progress tracking (e.g., 2/3 items collected)
struct SubObjective {
    std::string description;
    bool completed = false;
    int required = 1;
    int progress = 0;
};

// Dialogue/clue entry
struct DialogueEntry {
    std::string speaker;    // NPC or "environment"
    std::string text;
    std::string hint;       // Helpful hint for player
    bool revealed = false;
};

// Individual quest objective/step - ENHANCED
struct QuestObjective {
    std::string description;
    ObjectiveType type = ObjectiveType::Collect;
    bool completed = false;
    float progressPercent = 0.0f;  // 0-100
    
    // New: Sub-objectives for progress tracking
    std::vector<SubObjective> subObjectives;
    
    // New: Dialogue and clues
    std::vector<DialogueEntry> dialogues;
    std::vector<std::string> environmentalClues;
    
    // New: Skill requirements
    std::string requiredCharacter;  // Empty = any character
    int requiredSkillLevel = 0;
    
    // New: Consequences
    std::string successBranch;      // What happens on success
    std::string failureBranch;      // What happens on failure
    
    // Helpers
    int GetProgressCount() const {
        int count = 0;
        for (const auto& sub : subObjectives) {
            if (sub.completed) count++;
        }
        return count;
    }
    
    std::string GetProgressString() const {
        if (subObjectives.empty()) return "";
        int completed = GetProgressCount();
        return std::to_string(completed) + "/" + std::to_string(subObjectives.size());
    }
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
    int GetNextIncompleteObjectiveIndex() const;
    bool IsComplete() const { return state == QuestState::Complete; }
    bool IsFailed() const { return state == QuestState::Failed; }
    
    float GetProgress() const { return progressPercent; }
    const std::vector<QuestObjective>& GetObjectives() const { return objectives; }
    
    // Consequence: other characters get affected
    const std::string& GetConsequenceDescription() const { return consequenceDesc; }
    bool HasConsequence() const { return !consequenceDesc.empty(); }
    // NEW: Detailed quest info
    ObjectiveType GetCurrentObjectiveType(int index) const;
    std::string GetCurrentObjectiveHint(int index) const;
    const std::vector<DialogueEntry>& GetObjectiveDialogues(int index) const;
    const std::vector<std::string>& GetObjectiveClues(int index) const;
    std::string GetObjectiveProgress(int index) const;  // Returns "2/3" etc
    void ProgressSubObjective(int objectiveIndex, int subIndex);
    void RevealDialogue(int objectiveIndex, int dialogueIndex);
    
private:
    QuestType type;
    std::string title;
    QuestState state = QuestState::NotStarted;
    float progressPercent = 0.0f;  // 0-100
    std::vector<QuestObjective> objectives;
    std::string consequenceDesc;  // What happens to other characters
    
    void InitializeObjectives();  // Called in constructor to set up quest steps
};
