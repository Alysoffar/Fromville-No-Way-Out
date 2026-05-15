#pragma once

#include "game/quest/Quest.h"
#include "game/entities/Character.h"
#include <array>
#include <memory>
#include <string>
#include <vector>

enum class DayCyclePhase {
    Morning,
    Afternoon,
    Sunset,
    Night
};

// Quest progression stages
enum class StoryPhase {
    Exploration,      // Days 1-3: All characters gathering information
    Revelation,       // Day 4: Victor has a major memory breakthrough
    Confrontation,    // Days 5-7: Cult becomes aware of investigation
    Climax,           // Day 8: Final confrontation
    Epilogue          // Day 9+: Consequences resolve
};

class QuestSystem {
public:
    explicit QuestSystem(float* worldClockPtr);
    
    // Initialize quests for all 5 characters
    void Initialize(const std::array<Character*, 5>& characters);
    
    // Update quests based on game state
    void Update(float dt);
    
    // Get character's active quest
    Quest* GetCharacterQuest(CharacterType type);
    const Quest* GetCharacterQuest(CharacterType type) const;
    
    // Advance a specific objective
    void AdvanceObjective(CharacterType type, int objectiveIndex);
    
    // Get current story phase
    StoryPhase GetCurrentPhase() const { return currentPhase; }
    float GetDayNumber() const { return *worldClockPtr / 120.0f; }  // 120 seconds = 1 day
    static DayCyclePhase GetDayCyclePhaseFromClock(float worldClock);
    DayCyclePhase GetCurrentDayCyclePhase() const;
    bool IsProgressAllowed(CharacterType type, DayCyclePhase phase) const;
    std::string GetProgressWindowLabel(CharacterType type) const;
    std::string GetProgressLockMessage(CharacterType type, DayCyclePhase phase) const;
    
    // Check if there are pending consequences to broadcast
    bool HasPendingConsequences() const { return !pendingConsequences.empty(); }
    const std::string& GetNextConsequence();
    
    // Getters for quest states
    int GetCompletedQuestCount() const;
    bool AreAllQuestsComplete() const;

    // Simple persistent story flags used by interactions and triggers
    void SetStoryFlag(const std::string& flag);
    bool HasStoryFlag(const std::string& flag) const;
    void ClearStoryFlag(const std::string& flag);
    std::string SerializeState() const;
    void DeserializeState(const std::string& state);

private:
    std::array<std::unique_ptr<Quest>, 5> characterQuests;
    std::array<Character*, 5> characterPointers;
    float* worldClockPtr;
    float elapsedTime = 0.0f;
    StoryPhase currentPhase = StoryPhase::Exploration;
    int consequenceIndex = 0;
    std::vector<std::string> pendingConsequences;
    std::vector<std::string> storyFlags;
    std::array<bool, 5> questCompletionConsequenceTriggered = {false, false, false, false, false};
    
    // Story milestone tracking
    bool victorsMemoryTriggered = false;
    bool cultAwarenessTriggered = false;
    
    void UpdateStoryPhase();
    void CheckConsequences();
    void TriggerConsequence(const std::string& consequence);
};
