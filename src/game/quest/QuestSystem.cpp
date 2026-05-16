#include "game/quest/QuestSystem.h"

#include <algorithm>
#include <cmath>
#include <iostream>

#include <nlohmann/json.hpp>
#include "game/entities/Boyd.h"
#include "game/entities/Victor.h"
#include "game/dialogue/DialogueManager.h"

namespace {
std::string DayCyclePhaseToString(DayCyclePhase phase) {
    switch (phase) {
        case DayCyclePhase::Morning: return "MORNING";
        case DayCyclePhase::Afternoon: return "AFTERNOON";
        case DayCyclePhase::Sunset: return "SUNSET";
        case DayCyclePhase::Night: return "NIGHT";
    }
    return "DAY";
}
}

QuestSystem::QuestSystem(float* worldClockPtr)
    : worldClockPtr(worldClockPtr) {
    // Create quests for all 5 characters
    characterQuests[0] = std::make_unique<Quest>(QuestType::Boyd_SolveConspiracy, "Boyd's Quest");
    characterQuests[1] = std::make_unique<Quest>(QuestType::Jade_DecodeSymbols, "Jade's Quest");
    characterQuests[2] = std::make_unique<Quest>(QuestType::Tabitha_MapTunnels, "Tabitha's Quest");
    characterQuests[3] = std::make_unique<Quest>(QuestType::Victor_RecallPast, "Victor's Quest");
    // Sara's quest removed per user request; leave slot empty
}

DayCyclePhase QuestSystem::GetDayCyclePhaseFromClock(float worldClock) {
    const float cyclePosition = std::fmod(worldClock, 120.0f);
    if (cyclePosition < 40.0f) {
        return DayCyclePhase::Morning;
    }
    if (cyclePosition < 64.0f) {
        return DayCyclePhase::Afternoon;
    }
    if (cyclePosition < 72.0f) {
        return DayCyclePhase::Sunset;
    }
    return DayCyclePhase::Night;
}

DayCyclePhase QuestSystem::GetCurrentDayCyclePhase() const {
    return GetDayCyclePhaseFromClock(worldClockPtr ? *worldClockPtr : 0.0f);
}

bool QuestSystem::IsProgressAllowed(CharacterType type, DayCyclePhase phase) const {
    switch (type) {
        case CharacterType::Boyd:
            return phase == DayCyclePhase::Morning || phase == DayCyclePhase::Afternoon || phase == DayCyclePhase::Sunset;
        case CharacterType::Jade:
            return phase == DayCyclePhase::Night;
        case CharacterType::Tabitha:
            return phase == DayCyclePhase::Night;
        case CharacterType::Victor:
            return phase == DayCyclePhase::Morning || phase == DayCyclePhase::Afternoon;
        case CharacterType::Sara:
            return phase == DayCyclePhase::Morning || phase == DayCyclePhase::Afternoon || phase == DayCyclePhase::Sunset;
    }

    return false;
}

std::string QuestSystem::GetProgressWindowLabel(CharacterType type) const {
    switch (type) {
        case CharacterType::Boyd: return "DAY / SUNSET";
        case CharacterType::Jade: return "NIGHT";
        case CharacterType::Tabitha: return "NIGHT";
        case CharacterType::Victor: return "DAY";
        case CharacterType::Sara: return "DAY / SUNSET";
    }

    return "DAY";
}

std::string QuestSystem::GetProgressLockMessage(CharacterType type, DayCyclePhase phase) const {
    switch (type) {
        case CharacterType::Boyd:
            return phase == DayCyclePhase::Night ? "Boyd works before dark. Use the daylight or sunset window." : "";
        case CharacterType::Jade:
            return phase == DayCyclePhase::Night ? "The symbols are dormant. This changes after dark." : "The town hides it during the day.";
        case CharacterType::Tabitha:
            return phase == DayCyclePhase::Night ? "The tunnels are waiting for night. The echoes are gone during daylight." : "The tunnels have not changed yet.";
        case CharacterType::Victor:
            return phase == DayCyclePhase::Night ? "Victor sleeps through the night. The memories come in daylight." : "";
        case CharacterType::Sara:
            return phase == DayCyclePhase::Night ? "Sara's investigation calms down during the day. Night is for fear, not progress." : "";
    }

    return "";
}

void QuestSystem::Initialize(const std::array<Character*, 5>& characters) {
    characterPointers = characters;
    questCompletionConsequenceTriggered = {false, false, false, false, false};
    
    // Start all quests (skip empty slots)
    for (auto& quest : characterQuests) {
        if (quest) quest->Start();
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
        if (!quest) continue;
        if (!questCompletionConsequenceTriggered[i] && quest->IsComplete() && quest->HasConsequence()) {
            TriggerConsequence(quest->GetConsequenceDescription());
            questCompletionConsequenceTriggered[i] = true;
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

const Quest* QuestSystem::GetCharacterQuest(CharacterType type) const {
    int index = static_cast<int>(type);
    if (index >= 0 && index < 5) {
        return characterQuests[index].get();
    }
    return nullptr;
}

void QuestSystem::AdvanceObjective(CharacterType type, int objectiveIndex) {
    Quest* quest = GetCharacterQuest(type);
    if (quest) {
        const DayCyclePhase phase = GetCurrentDayCyclePhase();
        if (!IsProgressAllowed(type, phase)) {
            std::cout << "[Objective] Blocked by cycle phase " << DayCyclePhaseToString(phase)
                      << " for " << static_cast<int>(type) << "\n";
            return;
        }

        const int nextObjectiveIndex = quest->GetNextIncompleteObjectiveIndex();
        if (nextObjectiveIndex != objectiveIndex) {
            std::cout << "[Objective] Ignored out-of-order step for " << static_cast<int>(type)
                      << " (expected " << nextObjectiveIndex << ", got " << objectiveIndex << ")\n";
            return;
        }

        quest->CompleteObjective(objectiveIndex);
        
        // Log objective completion
        std::cout << "[Objective] Completed for " << static_cast<int>(type) << "\n";

        // Reward: small trust increase for the character whose objective completed
        auto CharacterTypeToName = [](CharacterType t) -> std::string {
            switch (t) {
                case CharacterType::Boyd: return "Boyd";
                case CharacterType::Jade: return "Jade";
                case CharacterType::Tabitha: return "Tabitha";
                case CharacterType::Victor: return "Victor";
                case CharacterType::Sara: return "Sara";
            }
            return "";
        };

        const std::string charName = CharacterTypeToName(type);
        if (!charName.empty()) {
            DialogueManager::Instance().ModifyTrust(charName, +6);
            DialogueManager::Instance().AddMemoryFlag(charName, std::string("objective:") + std::to_string(objectiveIndex));
        }
        
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
        if (quest && quest->IsComplete()) {
            ++count;
        }
    }
    return count;
}

bool QuestSystem::AreAllQuestsComplete() const {
    for (const auto& quest : characterQuests) {
        if (quest && !quest->IsComplete()) {
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

std::string QuestSystem::SerializeState() const {
    nlohmann::json json;
    json["currentPhase"] = static_cast<int>(currentPhase);
    json["elapsedTime"] = elapsedTime;
    json["victorsMemoryTriggered"] = victorsMemoryTriggered;
    json["cultAwarenessTriggered"] = cultAwarenessTriggered;
    json["storyFlags"] = storyFlags;

    for (const auto& quest : characterQuests) {
        if (quest) json["quests"].push_back(quest->SerializeState());
        else json["quests"].push_back(std::string());
    }

    return json.dump();
}

void QuestSystem::DeserializeState(const std::string& stateText) {
    if (stateText.empty()) {
        return;
    }

    try {
        const nlohmann::json json = nlohmann::json::parse(stateText);
        currentPhase = static_cast<StoryPhase>(json.value("currentPhase", static_cast<int>(currentPhase)));
        elapsedTime = json.value("elapsedTime", elapsedTime);
        victorsMemoryTriggered = json.value("victorsMemoryTriggered", victorsMemoryTriggered);
        cultAwarenessTriggered = json.value("cultAwarenessTriggered", cultAwarenessTriggered);
        storyFlags = json.value("storyFlags", storyFlags);

        const auto& quests = json.contains("quests") ? json["quests"] : nlohmann::json::array();
        for (std::size_t i = 0; i < characterQuests.size() && i < quests.size(); ++i) {
            if (characterQuests[i]) {
                characterQuests[i]->DeserializeState(quests[i].get<std::string>());
                questCompletionConsequenceTriggered[i] = characterQuests[i]->IsComplete();
            } else {
                questCompletionConsequenceTriggered[i] = false;
            }
        }
    } catch (...) {
        // Keep defaults when loading an invalid state payload.
    }
}
