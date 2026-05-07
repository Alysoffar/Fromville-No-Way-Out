#include "game/quest/Quest.h"

#include <array>
#include <iostream>

namespace {
struct QuestDefinition {
    const char* title;
    std::array<const char*, 5> objectives;
    const char* consequence;
};

constexpr std::array<QuestDefinition, 5> kQuestDefinitions = {{
    {
        "Solve the Conspiracy",
        {"Interrogate first suspect",
         "Gather 3 pieces of evidence",
         "Discover cult gathering place",
         "Confront the leader",
         "Prevent the ritual"},
        "As Boyd investigates, the cult becomes aware and sends hunters to all characters."
    },
    {
        "Decode the Symbols",
        {"Find glyph #1 (library)",
         "Find glyph #2 (town hall)",
         "Find glyph #3 (church)",
         "Find glyph #4 (tunnel entrance)",
         "Decipher the full message"},
        "Each decoded glyph gives Boyd a critical clue and unlocks a shortcut tunnel."
    },
    {
        "Map the Tunnel Network",
        {"Explore North tunnel",
         "Explore East tunnel",
         "Explore South tunnel",
         "Explore West tunnel",
         "Discover the central chamber"},
        "Each tunnel section unlocked lets Victor recall a memory from his past, revealing the cult's origin."
    },
    {
        "Recall the Past Events",
        {"Remember the first disappearance (1985)",
         "Remember the second disappearance (1990)",
         "Remember the cult gathering (1995)",
         "Remember the betrayal (2000)",
         "Remember the survivor's identity (now)"},
        "Each recalled memory is broadcast to all characters, advancing everyone's understanding of the truth."
    },
    {
        "Find Your Redeemer",
        {"Identify your torturer",
         "Gather testimonies (2 witnesses)",
         "Locate the crime scene",
         "Confront the perpetrator",
         "Choose justice or mercy"},
        "Each truth Sara uncovers increases creature awareness. A false accusation resets all clues and kills Sara."
    }
}};

const QuestDefinition* GetQuestDefinition(QuestType type) {
    const auto index = static_cast<std::size_t>(type);
    if (index >= kQuestDefinitions.size()) {
        return nullptr;
    }

    return &kQuestDefinitions[index];
}
}

Quest::Quest(QuestType type, std::string title)
    : type(type), title(std::move(title)), state(QuestState::NotStarted) {
    InitializeObjectives();
}

void Quest::Start() {
    if (state == QuestState::NotStarted) {
        state = QuestState::InProgress;
        std::cout << "[Quest] Started: " << title << "\n";
    }
}

void Quest::AdvanceProgress(float amount) {
    if (state != QuestState::InProgress) {
        return;
    }
    
    progressPercent += amount * 100.0f;  // amount is 0-1, convert to 0-100
    
    if (progressPercent >= 100.0f) {
        progressPercent = 100.0f;
        state = QuestState::Complete;
        std::cout << "[Quest] COMPLETE: " << title << "\n";
    } else if (progressPercent > 0.0f && progressPercent < 100.0f) {
        state = QuestState::PartialComplete;
    }
}

void Quest::CompleteObjective(int index) {
    if (index >= 0 && index < static_cast<int>(objectives.size())) {
        if (!objectives[index].completed) {
            objectives[index].completed = true;
            objectives[index].progressPercent = 100.0f;
            
            // Each completed objective is 20% progress (5 objectives = 100%)
            AdvanceProgress(0.20f);
            std::cout << "[Quest] Objective completed: " << objectives[index].description << "\n";
        }
    }
}

void Quest::InitializeObjectives() {
    const QuestDefinition* definition = GetQuestDefinition(type);
    if (!definition) {
        title = "Unknown Quest";
        consequenceDesc.clear();
        return;
    }

    title = definition->title;
    objectives.clear();
    objectives.reserve(definition->objectives.size());
    for (const char* objectiveDescription : definition->objectives) {
        objectives.emplace_back(QuestObjective{objectiveDescription, false, 0.0f});
    }
    consequenceDesc = definition->consequence;
}
