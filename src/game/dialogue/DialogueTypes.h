#pragma once

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include "game/quest/Quest.h"

enum class EmotionState {
    Neutral,
    Tense,
    Angry,
    Sad,
    Fearful,
    Calm
};

enum class DialogueTiming {
    Immediate,
    PauseShort,
    PauseLong,
    TimedResponse
};

struct DialogueCondition {
    // Simple condition predicate by key — evaluated by DialogueManager at runtime
    std::string flag; // story/relationship/memory flag
    bool requiredValue = true;
};

struct DialogueChoice {
    std::string id;
    std::string text;
    std::string nextNodeId; // empty = end
    int trustDelta = 0; // modifies relationship/trust
    std::vector<DialogueCondition> conditions;
};

struct DialogueNode {
    std::string id;
    std::string speaker; // display name
    std::string text;
    EmotionState emotion = EmotionState::Neutral;
    DialogueTiming timing = DialogueTiming::Immediate;
    float preDelaySeconds = 0.0f;
    float postDelaySeconds = 0.35f;
    float revealCharactersPerSecond = 42.0f;
    bool allowSkip = true;
    bool interruptible = true;
    bool autoAdvance = false;
    std::string nextNodeId;
    std::vector<DialogueChoice> choices; // player choices
    std::vector<DialogueCondition> conditions; // node availability
    bool isPlayerNode = false; // true when player speaks (choice selection)
    // optional voice grouping for audio: e.g., "npc_male", "npc_female", "responses"
    std::string voiceGroup;
};

struct Conversation {
    std::string id;
    std::string title;
    std::unordered_map<std::string, DialogueNode> nodes; // indexed by node id
    std::string startNode;
};

struct RelationshipState {
    int trust = 0; // -100 .. +100
    std::vector<std::string> memoryFlags;
    std::vector<std::string> dialogueHistory;
    EmotionState lastEmotion = EmotionState::Neutral;
};
