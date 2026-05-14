#pragma once

#include <functional>
#include <string>

#include "engine/input/InputContext.h"

class TextRenderer;

class PuzzleBase {
public:
    using SoundHook = std::function<void(const std::string&)>;
    using ConsequenceHook = std::function<void(const std::string&)>;

    virtual ~PuzzleBase() = default;

    void SetSoundHook(SoundHook hook) { soundHook = std::move(hook); }
    void SetConsequenceHook(ConsequenceHook hook) { consequenceHook = std::move(hook); }

    virtual void Start() = 0;
    virtual void Update(float dt, const InputContext& input) = 0;
    virtual void Render(TextRenderer& textRenderer, int screenWidth, int screenHeight, float alpha) const = 0;
    virtual bool IsSolved() const = 0;
    virtual bool IsReady() const { return true; }
    virtual std::string GetTitle() const = 0;
    virtual std::string GetClueText() const = 0;
    virtual std::string GetSolvedMessage() const = 0;
    virtual std::string SerializeState() const = 0;
    virtual void DeserializeState(const std::string& state) = 0;
    virtual void Reset() = 0;
    virtual bool IsCorrupted() const { return false; }

    // Optional: return currently selected index for UI (1-based expected); -1 if none
    virtual int GetSelectedIndex() const { return -1; }

protected:
    void PlaySound(const std::string& cue) const {
        if (soundHook) {
            soundHook(cue);
        }
    }

    void EmitConsequence(const std::string& consequence) const {
        if (consequenceHook) {
            consequenceHook(consequence);
        }
    }

private:
    SoundHook soundHook;
    ConsequenceHook consequenceHook;
};