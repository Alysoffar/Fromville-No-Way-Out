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

// Helper to build detailed objectives with all features
QuestObjective BuildObjective(
    const std::string& description,
    ObjectiveType type,
    const std::vector<SubObjective>& subs = {},
    const std::vector<DialogueEntry>& dialogues = {},
    const std::vector<std::string>& clues = {},
    const std::string& requiredChar = "",
    const std::string& successBranch = "",
    const std::string& failBranch = "") {
    QuestObjective obj;
    obj.description = description;
    obj.type = type;
    obj.subObjectives = subs;
    obj.dialogues = dialogues;
    obj.environmentalClues = clues;
    obj.requiredCharacter = requiredChar;
    obj.successBranch = successBranch;
    obj.failureBranch = failBranch;
    return obj;
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

int Quest::GetNextIncompleteObjectiveIndex() const {
    for (int index = 0; index < static_cast<int>(objectives.size()); ++index) {
        if (!objectives[index].completed) {
            return index;
        }
    }

    return -1;
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
    consequenceDesc = definition->consequence;
    
    // Initialize with DETAILED features for each character
    if (type == QuestType::Boyd_SolveConspiracy) {
        // BOYD: Detective story with NPC interactions, evidence collection, and combat
        objectives.clear();
        
        // 1. Dialogue + Information gathering
        objectives.push_back(BuildObjective(
            "Interrogate first suspect",
            ObjectiveType::Dialogue,
            {{SubObjective{"Talk to Mara", false, 1, 0}}},
            {
                {DialogueEntry{"Mara", "I saw strange symbols on the church wall last week...", "Listen for clues about cult activity"}},
                {DialogueEntry{"Mara", "Three people have vanished in the last month. The sheriff won't investigate.", "People are disappearing"}}
            },
            {"Mara saw someone leaving the cult site at midnight"}
        ));
        
        // 2. Collect + Multi-part objective
        objectives.push_back(BuildObjective(
            "Gather 3 pieces of evidence",
            ObjectiveType::Collect,
            {
                {SubObjective{"Bloody knife at warehouse", false, 1, 0}},
                {SubObjective{"Cult ledger (hidden)", false, 1, 0}},
                {SubObjective{"Strange idol", false, 1, 0}}
            },
            {},
            {
                "Knife has cult symbols on handle",
                "Ledger lists names of town leaders",
                "Idol matches the church symbols"
            }
        ));
        
        // 3. Puzzle + Environmental storytelling
        objectives.push_back(BuildObjective(
            "Discover cult gathering place",
            ObjectiveType::Puzzle,
            {{SubObjective{"Solve the symbol puzzle", false, 1, 0}}},
            {},
            {
                "Five identical symbols at: church, library, town hall, old mill, cemetery",
                "Arranging them reveals a map to the cult headquarters",
                "The gathering place is beneath the old cemetery chapel"
            }
        ));
        
        // 4. Combat + Timed challenge
        objectives.push_back(BuildObjective(
            "Confront the leader",
            ObjectiveType::Combat,
            {{SubObjective{"Defeat the cult leader", false, 1, 0}}, {SubObjective{"Obtain confession", false, 1, 0}}},
            {
                {DialogueEntry{"Leader", "You should have stayed away, detective.", "This is the cult's true leader"}}
            },
            {"Leader reveals all cult operations when defeated"}
        ));
        
        // 5. Combat + Choice-based consequence
        objectives.push_back(BuildObjective(
            "Prevent the ritual",
            ObjectiveType::Combat,
            {{SubObjective{"Stop the sacrifice", false, 1, 0}}, {SubObjective{"Rescue victim", false, 1, 0}}},
            {},
            {
                "The cult needs 7 people for the ritual to work",
                "Stopping it now saves this victim but delays cult by 1 day",
                "Completing the mission ends the cult threat permanently"
            },
            "",
            "Cult is destroyed. Other characters can relax.",
            "Cult escapes. Threat escalates. Creature attacks increase."
        ));
        
    } else if (type == QuestType::Jade_DecodeSymbols) {
        // JADE: Puzzle-solving with environmental clues and skill checks
        objectives.clear();
        
        objectives.push_back(BuildObjective(
            "Find glyph #1 (library)",
            ObjectiveType::Environmental,
            {{SubObjective{"Decode the symbol", false, 1, 0}}, {SubObjective{"Sketch it", false, 1, 0}}},
            {},
            {
                "Located on old bookshelf behind restricted section",
                "Represents 'Knowledge' - circular with rays",
                "Only Jade can fully understand ancient languages"
            },
            "Jade"  // Requires Jade
        ));
        
        objectives.push_back(BuildObjective(
            "Find glyph #2 (town hall)",
            ObjectiveType::Puzzle,
            {{SubObjective{"Find the glyph", false, 1, 0}}, {SubObjective{"Photograph it", false, 1, 0}}},
            {},
            {
                "Hidden in town records - look for old maps",
                "Represents 'Power' - interlocking triangles",
                "Symbol connects to town founder's diary"
            }
        ));
        
        objectives.push_back(BuildObjective(
            "Find glyph #3 (church)",
            ObjectiveType::Observe,
            {{SubObjective{"Sneak into church at night", false, 1, 0}}, {SubObjective{"Document symbol", false, 1, 0}}},
            {},
            {
                "High in the stained glass window",
                "Only visible at midnight when moonlight hits it",
                "Represents 'Truth' - eye within pyramid"
            }
        ));
        
        objectives.push_back(BuildObjective(
            "Find glyph #4 (tunnel entrance)",
            ObjectiveType::Collect,
            {{SubObjective{"Locate entrance", false, 1, 0}}, {SubObjective{"Collect rubbing", false, 1, 0}}},
            {},
            {
                "Behind waterfall at canyon's edge",
                "Represents 'Crossing' - two circles meeting",
                "Leads to secret tunnel network below town"
            }
        ));
        
        objectives.push_back(BuildObjective(
            "Decipher the full message",
            ObjectiveType::Puzzle,
            {{SubObjective{"Arrange all 4 glyphs", false, 1, 0}}, {SubObjective{"Read the prophecy", false, 1, 0}}},
            {
                {DialogueEntry{"Jade's visions", "The four symbols... they speak of a ritual every 25 years...", "This prophecy repeats"}}
            },
            {"Complete message: 'The crossing opens once every generation. Power seeks to awaken. Truth must be guarded or chaos breaks free.'"}
        ));
        
    } else if (type == QuestType::Tabitha_MapTunnels) {
        // TABITHA: Exploration with skill requirements and mapping
        objectives.clear();
        
        objectives.push_back(BuildObjective(
            "Explore North tunnel",
            ObjectiveType::Observe,
            {{SubObjective{"Map the tunnel", false, 1, 0}}, {SubObjective{"Find compass marker", false, 1, 0}}, {SubObjective{"Return safely", false, 1, 0}}},
            {},
            {
                "Ancient stone carved with N",
                "Tunnel contains cult symbols on walls",
                "Dead end but holds important diary entry"
            }
        ));
        
        objectives.push_back(BuildObjective(
            "Explore East tunnel",
            ObjectiveType::Combat,
            {{SubObjective{"Defeat creatures", false, 1, 0}}, {SubObjective{"Mark tunnel", false, 1, 0}}},
            {},
            {
                "Most dangerous section - creatures nest here",
                "Contains ancient mining equipment",
                "Marker: E carved with blood (recent)"
            },
            "",
            "Creatures abandon this tunnel section",
            "Creatures hunt Tabitha constantly"
        ));
        
        objectives.push_back(BuildObjective(
            "Explore South tunnel",
            ObjectiveType::Puzzle,
            {{SubObjective{"Navigate flooded section", false, 1, 0}}, {SubObjective{"Find S marker", false, 1, 0}}},
            {},
            {
                "Water-filled chambers require clever navigation",
                "Contains ancient sculptures",
                "Marker hidden behind rockfall"
            }
        ));
        
        objectives.push_back(BuildObjective(
            "Explore West tunnel",
            ObjectiveType::Collect,
            {{SubObjective{"Gather mineral samples", false, 1, 0}}, {SubObjective{"Find W marker", false, 1, 0}}},
            {},
            {
                "Contains rare glowing crystals",
                "Crystals align with constellation markers",
                "Marker: W made of crystal"
            }
        ));
        
        objectives.push_back(BuildObjective(
            "Discover the central chamber",
            ObjectiveType::Environmental,
            {{SubObjective{"Locate the center", false, 1, 0}}, {SubObjective{"Decode the central altar", false, 1, 0}}},
            {
                {DialogueEntry{"Ancient inscription", "Four paths lead to truth. Power flows from below. When the markers align, the veil thins.", "This prophecy is old"}}
            },
            {"Altar shows ritual components: blood, crystal, symbol, sacrifice"}
        ));
        
    } else if (type == QuestType::Victor_RecallPast) {
        // VICTOR: Memory-based with triggered dialogue and skill checks
        objectives.clear();
        
        objectives.push_back(BuildObjective(
            "Remember the first disappearance (1985)",
            ObjectiveType::Dialogue,
            {{SubObjective{"Listen to the memory", false, 1, 0}}, {SubObjective{"Verify the details", false, 1, 0}}},
            {
                {DialogueEntry{"Victor's memory", "A girl from school... Sarah... the cult took her for the ritual...", "This trauma is deep"}},
                {DialogueEntry{"Victor's subconscious", "I should have told someone. I was too afraid.", "Victor blames himself"}}
            },
            {"First cult victim was Victor's classmate. Cult leader was his mentor."},
            "Victor"
        ));
        
        objectives.push_back(BuildObjective(
            "Remember the second disappearance (1990)",
            ObjectiveType::Dialogue,
            {{SubObjective{"Face the memory", false, 1, 0}}, {SubObjective{"Accept the guilt", false, 1, 0}}},
            {
                {DialogueEntry{"Victor's memory", "Another girl. I saw the leader recruiting her. And I... I said nothing.", "Guilt consumes him"}}
            },
            {"Victor witnessed the recruitment but was too afraid to warn anyone"},
            "Victor",
            "Victor resolves to stop them this time",
            "Victor withdraws into himself"
        ));
        
        objectives.push_back(BuildObjective(
            "Remember the cult gathering (1995)",
            ObjectiveType::Puzzle,
            {{SubObjective{"Recall the location", false, 1, 0}}, {SubObjective{"Remember the ritual", false, 1, 0}}},
            {
                {DialogueEntry{"Victor's memory", "The ceremony happened in the cemetery chapel. I heard chanting. I was outside... watching. Paralyzed with fear.", "This is the key moment"}}
            },
            {"Victor discovered the cult's gathering place but never reported it"},
            "Victor"
        ));
        
        objectives.push_back(BuildObjective(
            "Remember the betrayal (2000)",
            ObjectiveType::Dialogue,
            {{SubObjective{"Recall who betrayed him", false, 1, 0}}, {SubObjective{"Understand why", false, 1, 0}}},
            {
                {DialogueEntry{"Victor's memory", "The leader... told me I was chosen. That together we would unlock the prophecy. For a moment, I believed him.", "He was almost tempted"}}
            },
            {"The cult leader tried to recruit Victor as his successor"},
            "Victor"
        ));
        
        objectives.push_back(BuildObjective(
            "Remember the survivor's identity (now)",
            ObjectiveType::Puzzle,
            {{SubObjective{"Identify the survivor", false, 1, 0}}, {SubObjective{"Find them", false, 1, 0}}},
            {
                {DialogueEntry{"Victor", "Wait... the young woman who came to town last month... she looks familiar... it's Sarah. She survived.", "Could it be?"}}
            },
            {"One victim escaped 25 years ago and has returned to stop the cycle"},
            "Victor"
        ));
        
    } else if (type == QuestType::Sara_FindRedemption) {
        // SARA: Investigation with moral choices and skill checks
        objectives.clear();
        
        objectives.push_back(BuildObjective(
            "Identify your torturer",
            ObjectiveType::Observe,
            {{SubObjective{"Gather descriptions", false, 1, 0}}, {SubObjective{"Identify the person", false, 1, 0}}},
            {
                {DialogueEntry{"Sara's intuition", "He had a scar on his left hand. And that voice... I could never forget it.", "Description is clear"}}
            },
            {"Evidence points to Thomas Reed, currently sheriff"}
        ));
        
        objectives.push_back(BuildObjective(
            "Gather testimonies (2 witnesses)",
            ObjectiveType::Dialogue,
            {
                {SubObjective{"First witness", false, 1, 0}},
                {SubObjective{"Second witness", false, 1, 0}}
            },
            {},
            {
                "Old doctor remembers treating cult torture victims in 1995",
                "Former cult member will testify against Reed in exchange for protection"
            }
        ));
        
        objectives.push_back(BuildObjective(
            "Locate the crime scene",
            ObjectiveType::Environmental,
            {{SubObjective{"Find the basement", false, 1, 0}}, {SubObjective{"Collect evidence", false, 1, 0}}, {SubObjective{"Document it", false, 1, 0}}},
            {},
            {
                "Basement beneath old Reed family house",
                "Contains torture equipment and cult symbols",
                "Evidence: blood samples, journals, victim photographs"
            }
        ));
        
        objectives.push_back(BuildObjective(
            "Confront the perpetrator",
            ObjectiveType::Combat,
            {{SubObjective{"Survive the confrontation", false, 1, 0}}, {SubObjective{"Get confession", false, 1, 0}}},
            {
                {DialogueEntry{"Reed", "You should have stayed forgotten, Sara. Now I have to silence you.", "He's not sorry"}}
            },
            {"Reed becomes extremely dangerous when cornered"}
        ));
        
        objectives.push_back(BuildObjective(
            "Choose justice or mercy",
            ObjectiveType::Puzzle,
            {{SubObjective{"Make your choice", false, 1, 0}}, {SubObjective{"Live with consequences", false, 1, 0}}},
            {
                {DialogueEntry{"Sara", "Do I destroy him like he destroyed me? Or do I let the law handle it?", "This defines who she is now"}}
            },
            {},
            "",
            "Sara finds closure. Cult is exposed to authorities. Other characters get justice.",
            "Sara's vengeance creates new enemies. Cult's backup leader takes power."
        ));
    }
    
    consequenceDesc = definition->consequence;

}

// NEW: Detailed objective type lookup
ObjectiveType Quest::GetCurrentObjectiveType(int index) const {
    if (index >= 0 && index < static_cast<int>(objectives.size())) {
        return objectives[index].type;
    }
    return ObjectiveType::Collect;
}

// NEW: Get helpful hint for current objective
std::string Quest::GetCurrentObjectiveHint(int index) const {
    if (index < 0 || index >= static_cast<int>(objectives.size())) {
        return "";
    }
    
    const auto& obj = objectives[index];
    switch (obj.type) {
        case ObjectiveType::Dialogue:
            return "Talk to the NPC and listen carefully";
        case ObjectiveType::Collect:
            return "Collect all items marked with red triangles";
        case ObjectiveType::Puzzle:
            return "Solve the puzzle by examining clues";
        case ObjectiveType::Observe:
            return "Sneak close without alerting enemies";
        case ObjectiveType::Combat:
            return "Defeat the enemies in combat";
        case ObjectiveType::Timed:
            return "Complete before time runs out!";
        case ObjectiveType::Skill:
            if (!obj.requiredCharacter.empty()) {
                return "Requires " + obj.requiredCharacter + "'s skills";
            }
            return "Use your character's unique skill";
        case ObjectiveType::Environmental:
            return "Search the environment for hidden clues";
    }
    return "";
}

// NEW: Get dialogues for this objective
const std::vector<DialogueEntry>& Quest::GetObjectiveDialogues(int index) const {
    static const std::vector<DialogueEntry> empty;
    if (index < 0 || index >= static_cast<int>(objectives.size())) {
        return empty;
    }
    return objectives[index].dialogues;
}

// NEW: Get clues for this objective
const std::vector<std::string>& Quest::GetObjectiveClues(int index) const {
    static const std::vector<std::string> empty;
    if (index < 0 || index >= static_cast<int>(objectives.size())) {
        return empty;
    }
    return objectives[index].environmentalClues;
}

// NEW: Get progress string like "2/3"
std::string Quest::GetObjectiveProgress(int index) const {
    if (index < 0 || index >= static_cast<int>(objectives.size())) {
        return "";
    }
    return objectives[index].GetProgressString();
}

// NEW: Progress a sub-objective
void Quest::ProgressSubObjective(int objectiveIndex, int subIndex) {
    if (objectiveIndex >= 0 && objectiveIndex < static_cast<int>(objectives.size())) {
        auto& obj = objectives[objectiveIndex];
        if (subIndex >= 0 && subIndex < static_cast<int>(obj.subObjectives.size())) {
            auto& sub = obj.subObjectives[subIndex];
            if (sub.progress < sub.required) {
                sub.progress++;
                if (sub.progress >= sub.required) {
                    sub.completed = true;
                }
            }
        }
    }
}

// NEW: Reveal dialogue
void Quest::RevealDialogue(int objectiveIndex, int dialogueIndex) {
    if (objectiveIndex >= 0 && objectiveIndex < static_cast<int>(objectives.size())) {
        auto& obj = objectives[objectiveIndex];
        if (dialogueIndex >= 0 && dialogueIndex < static_cast<int>(obj.dialogues.size())) {
            obj.dialogues[dialogueIndex].revealed = true;
        }
    }
}
