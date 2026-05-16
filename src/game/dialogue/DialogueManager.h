#pragma once

#include "DialogueTypes.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <iostream>

class DialogueUI;
class TextRenderer;
class InputContext;

class DialogueManager {
public:
    static DialogueManager& Instance();

    void Initialize();
    void LoadConversationsFromFolder(const std::string& folder = "assets/dialogue/");
    void SetAudioManager(class AudioManager* audio);
    bool StartConversation(const std::string& convoId);
    void Update(float dt);
    void HandleInput(const InputContext& input);
    void SelectChoice(int index);
    void CloseConversation();
    bool IsActive() const { return active; }
    const Conversation* GetActiveConversation() const { return activeConvo ? activeConvo : nullptr; }
    void Render(TextRenderer* hud, int screenWidth, int screenHeight) const;

    // relationship access
    RelationshipState& GetRelationship(const std::string& character) { return relationships[character]; }
    // Persistence helpers
    std::string SerializeState() const;
    void DeserializeState(const std::string& payload);

    // Runtime relationship APIs
    int GetTrust(const std::string& character) const;
    void ModifyTrust(const std::string& character, int delta);
    void SetTrust(const std::string& character, int value);

    void AddMemoryFlag(const std::string& character, const std::string& flag);
    void RemoveMemoryFlag(const std::string& character, const std::string& flag);
    bool HasMemoryFlag(const std::string& character, const std::string& flag) const;

    void MarkPromise(const std::string& character, const std::string& promiseId);
    void BreakPromise(const std::string& character, const std::string& promiseId);
    void MarkAccusation(const std::string& character, const std::string& accusationId);

    const RelationshipState* GetRelationshipState(const std::string& character) const;
    // Access relationship by index for debug UI. Returns pair of name and pointer (null if invalid).
    std::pair<std::string, RelationshipState*> GetRelationshipStateIndex(int index);
    std::pair<std::string, const RelationshipState*> GetRelationshipStateIndex(int index) const;
    int GetRelationshipCount() const { return static_cast<int>(relationships.size()); }
    // Debug utilities
    void DumpStateToFile(const std::string& path = "debug_dialogue_state.json") const;

private:
    DialogueManager();
    void BuildSampleConversations();

    class AudioManager* audioManager = nullptr;

    std::unordered_map<std::string, Conversation> conversations;
    Conversation* activeConvo = nullptr;
    std::string currentNodeId;
    bool active = false;
    std::unique_ptr<DialogueUI> ui;
    std::unordered_map<std::string, RelationshipState> relationships;
};
