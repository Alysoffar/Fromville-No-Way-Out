#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

class Character;
class GlyphRegistry;
class InputManager;
class NarrativeEngine;
class Renderer;

// ---------------------------------------------------------------------------
// DialogueResponse — one selectable reply in a branching dialogue
// ---------------------------------------------------------------------------
struct DialogueResponse {
    std::string text;

    /// Flag that must be set (in NarrativeEngine) for this response to appear.
    /// Empty string = always visible.
    std::string requiresFlag;

    /// Line ID to jump to when selected. Empty = ends dialogue.
    std::string nextLineId;

    /// Flag to set in NarrativeEngine when this response is selected.
    std::string setsFlag;

    /// Delta applied to Sara's redemption score when selected.
    float addsSaraRedemption = 0.0f;
};

// ---------------------------------------------------------------------------
// DialogueLine — one NPC speech node with optional branching responses
// ---------------------------------------------------------------------------
struct DialogueLine {
    std::string speaker;
    std::string text;

    std::vector<DialogueResponse> responses;

    /// Flag that must be set for this line to be reachable.
    /// Empty = always reachable.
    std::string requiresFlag;

    /// Flag to set when this line is first displayed.
    std::string setsFlag;

    /// If true, conversation ends automatically after displaying this line.
    bool endsDialogue = false;
};

// ---------------------------------------------------------------------------
// DialogueSystem — manages NPC conversations with flag-gated branches,
//                  a typewriter text-reveal effect, and keyboard navigation.
// ---------------------------------------------------------------------------
class DialogueSystem {
public:
    DialogueSystem(GlyphRegistry* glyphs, NarrativeEngine* narrative);

    // -----------------------------------------------------------------------
    // Data loading
    // -----------------------------------------------------------------------

    /// Load all dialogue trees from a JSON file.
    /// Expected structure:
    ///   { "npc_name": { "line_id": { speaker, text, responses, … }, … }, … }
    void LoadDialogues(const std::string& jsonPath);

    // -----------------------------------------------------------------------
    // Conversation control
    // -----------------------------------------------------------------------

    void StartConversation(const std::string& npcName, Character* player);
    void EndConversation();

    // -----------------------------------------------------------------------
    // Per-frame hooks (call from game loop)
    // -----------------------------------------------------------------------

    /// Advance typewriter timer and handle input (Up/Down/Enter).
    void Update(float dt, InputManager& input);

    /// Draw the dialogue box + text + response list.
    void Draw(Renderer& renderer);

    // -----------------------------------------------------------------------
    // Query
    // -----------------------------------------------------------------------

    bool IsActive() const { return active_; }

private:
    GlyphRegistry*   glyphs_;
    NarrativeEngine* narrative_;

    // Nested map: npcName -> lineId -> DialogueLine
    std::unordered_map<std::string,
        std::unordered_map<std::string, DialogueLine>> dialogues_;

    bool       active_          = false;
    std::string currentNPC_;
    std::string currentLineId_;
    Character*  currentPlayer_  = nullptr;

    float textRevealTimer_  = 0.0f;
    int   revealedChars_    = 0;
    int   selectedResponse_ = 0;

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------

    DialogueLine* GetLine(const std::string& lineId);

    /// Returns true if the flag is satisfied by the NarrativeEngine.
    /// An empty flag string always returns true.
    bool FlagSatisfied(const std::string& flag) const;

    /// Apply the response: set flags, apply redemption, advance line.
    void SelectResponse(int index);

    /// Render the dialogue box background + speaker name + body text.
    void DrawDialogueBox(Renderer& renderer, const DialogueLine& line);
};
