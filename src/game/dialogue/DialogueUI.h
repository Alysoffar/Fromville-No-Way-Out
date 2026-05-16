#pragma once

#include "DialogueTypes.h"
#include <string>

class TextRenderer;

class DialogueUI {
public:
    DialogueUI();
    void OpenConversation(const Conversation& convo, const std::string& startNodeId);
    void Update(float dt);
    void Render(TextRenderer* hud, int screenWidth, int screenHeight) const;
    void AdvanceToNode(const std::string& nodeId);
    void MoveSelection(int delta);
    void SetSelectedChoiceIndex(int index);
    int GetSelectedChoiceIndex() const { return selectedChoiceIndex; }
    bool IsFullyRevealed() const { return fullyRevealed; }
    bool IsWaitingForInput() const;
    void RevealFully();
    void Close();
    bool IsOpen() const { return open; }
    bool IsClosing() const { return closing; }
    const DialogueNode* GetCurrentNode() const;
    bool HasChoices() const;

private:
    void ResetLineState();
    float GetPunctuationSilence(const std::string& text) const;
    std::string WrapText(const std::string& text, std::size_t maxLineChars) const;
    void EnsureOverlayResources() const;
    void RenderOverlay(float alpha, int screenWidth, int screenHeight) const;

    const Conversation* activeConvo = nullptr;
    std::string currentNodeId;
    bool open = false;
    bool closing = false;
    float overlayAlpha = 0.0f;
    float preDelayRemaining = 0.0f;
    float postDelayRemaining = 0.0f;
    float revealProgress = 0.0f;
    bool fullyRevealed = false;
    int selectedChoiceIndex = 0;
};
