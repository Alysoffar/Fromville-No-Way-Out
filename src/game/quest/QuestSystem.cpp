#include "game/quest/QuestSystem.h"

#include <algorithm>
#include <iostream>
#include "game/entities/Boyd.h"
#include "game/entities/Victor.h"

QuestSystem::QuestSystem(float* worldClockPtr)
    : worldClockPtr(worldClockPtr) {
    // Create quests for all 5 characters
    characterQuests[0] = std::make_unique<Quest>(QuestType::Boyd_SolveConspiracy, "Boyd's Quest");
    characterQuests[1] = std::make_unique<Quest>(QuestType::Jade_DecodeSymbols, "Jade's Quest");
    characterQuests[2] = std::make_unique<Quest>(QuestType::Tabitha_MapTunnels, "Tabitha's Quest");
    characterQuests[3] = std::make_unique<Quest>(QuestType::Victor_RecallPast, "Victor's Quest");
    characterQuests[4] = std::make_unique<Quest>(QuestType::Sara_FindRedemption, "Sara's Quest");
}

void QuestSystem::Initialize(const std::array<Character*, 5>& characters) {
    characterPointers = characters;
    
    // Start all quests
    for (auto& quest : characterQuests) {
        quest->Start();
    }
    
    std::cout << "[QuestSystem] All character quests initialized and started.\n";
}

void QuestSystem::Update(float dt) {
    elapsedTime += dt;
    
    // Update story phase based on in-game days
    UpdateStoryPhase();
    
    // Check for story-triggered consequences
    CheckConsequences();
}

void QuestSystem::UpdateStoryPhase() {
    float dayNumber = GetDayNumber();
    
    // Phase transitions
    if (dayNumber < 3.0f && currentPhase != StoryPhase::Exploration) {
        currentPhase = StoryPhase::Exploration;
        std::cout << "[Story] PHASE: Exploration (Days 1-3) - All characters gather information\n";
    }
    else if (dayNumber >= 3.0f && dayNumber < 5.0f && currentPhase == StoryPhase::Exploration) {
        currentPhase = StoryPhase::Revelation;
        std::cout << "[Story] PHASE: Revelation (Day 4) - Victor remembers the truth!\n";
        
        // Trigger Victor's memory breakthrough
        if (!victorsMemoryTriggered) {
            victorsMemoryTriggered = true;
            Victor* victor = dynamic_cast<Victor*>(characterPointers[3]);
            if (victor) {
                std::cout << "[Victor] Memory surge! The past comes flooding back...\n";
            }
            TriggerConsequence("Victor's memory is unlocked. The cult's origin story is revealed to all characters.");
        }
    }
    else if (dayNumber >= 5.0f && dayNumber < 8.0f && currentPhase != StoryPhase::Confrontation) {
        currentPhase = StoryPhase::Confrontation;
        std::cout << "[Story] PHASE: Confrontation (Days 5-7) - Cult becomes aware!\n";
        
        // Trigger cult awareness
        if (!cultAwarenessTriggered) {
            cultAwarenessTriggered = true;
            Boyd* boyd = dynamic_cast<Boyd*>(characterPointers[0]);
            if (boyd) {
                boyd->AddCurseCharge(30.0f);  // Boyd senses the danger
                std::cout << "[Boyd] Something's wrong. The curse is rising...\n";
            }
            TriggerConsequence("The cult is aware of the investigation! Hunters begin stalking all characters. Creature encounters increase.");
        }
    }
    else if (dayNumber >= 8.0f && currentPhase != StoryPhase::Climax) {
        currentPhase = StoryPhase::Climax;
        std::cout << "[Story] PHASE: Climax (Day 8) - Final confrontation!\n";
    }
}

void QuestSystem::CheckConsequences() {
    // Check if any character completed a major quest
    for (size_t i = 0; i < characterQuests.size(); ++i) {
        Quest* quest = characterQuests[i].get();
        if (quest->IsComplete() && quest->HasConsequence()) {
            TriggerConsequence(quest->GetConsequenceDescription());
        }
    }
}

Quest* QuestSystem::GetCharacterQuest(CharacterType type) {
    int index = static_cast<int>(type);
    if (index >= 0 && index < 5) {
        return characterQuests[index].get();
    }
    return nullptr;
}

void QuestSystem::AdvanceObjective(CharacterType type, int objectiveIndex) {
    Quest* quest = GetCharacterQuest(type);
    if (quest) {
        quest->CompleteObjective(objectiveIndex);
        
        // Log objective completion
        std::cout << "[Objective] Completed for " << static_cast<int>(type) << "\n";
        
        // Add consequence based on objective
        auto objectives = quest->GetObjectives();
        if (objectiveIndex < static_cast<int>(objectives.size())) {
            TriggerConsequence("Objective completed: " + objectives[objectiveIndex].description);
        }
    }
}

int QuestSystem::GetCompletedQuestCount() const {
    int count = 0;
    for (const auto& quest : characterQuests) {
        if (quest->IsComplete()) {
            ++count;
        }
    }
    return count;
}

bool QuestSystem::AreAllQuestsComplete() const {
    for (const auto& quest : characterQuests) {
        if (!quest->IsComplete()) {
            return false;
        }
    }
    return true;
}

void QuestSystem::SetStoryFlag(const std::string& flag) {
    if (flag.empty() || HasStoryFlag(flag)) {
        return;
    }

    storyFlags.push_back(flag);
}

bool QuestSystem::HasStoryFlag(const std::string& flag) const {
    return std::find(storyFlags.begin(), storyFlags.end(), flag) != storyFlags.end();
}

void QuestSystem::ClearStoryFlag(const std::string& flag) {
    storyFlags.erase(std::remove(storyFlags.begin(), storyFlags.end(), flag), storyFlags.end());
}

const std::string& QuestSystem::GetNextConsequence() {
    static const std::string empty;
    if (pendingConsequences.empty()) {
        return empty;
    }
    
    const std::string& consequence = pendingConsequences[consequenceIndex];
    consequenceIndex++;
    
    if (consequenceIndex >= static_cast<int>(pendingConsequences.size())) {
        consequenceIndex = 0;
        pendingConsequences.clear();
    }
    
    return consequence;
}

void QuestSystem::TriggerConsequence(const std::string& consequence) {
    // Check if this consequence is already pending
    for (const auto& pending : pendingConsequences) {
        if (pending == consequence) {
            return;  // Don't add duplicates
        }
    }
    
    pendingConsequences.emplace_back(consequence);
    std::cout << "[Consequence] " << consequence << "\n";
}
