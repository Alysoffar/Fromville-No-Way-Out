#pragma once

#include <functional>
#include <vector>
#include <string>

class AudioManager;

class TransitionManager {
public:
    using Callback = std::function<void()>;

    static TransitionManager& Instance();

    void Update(float dt);

    // simple fade in/out for now
    void StartFade(float duration, bool fadeIn, Callback onComplete = {});

    // audio integration
    void SetAudioManager(AudioManager* a);
    void StartAudioFade(const std::string& cueName, float targetGain, float duration);
    void PlayStinger(const std::string& cueName, float gain = 1.0f);

    float GetFadeAlpha() const { return currentAlpha; }
    bool IsFading() const { return fading; }

private:
    TransitionManager();
    float currentAlpha = 0.0f;
    float fadeDuration = 0.0f;
    float fadeTimer = 0.0f;
    bool fadeIn = true;
    bool fading = false;
    Callback completeCb;

    // audio
    AudioManager* audio = nullptr;
    struct AudioFade {
        std::string cue;
        float startGain = 1.0f;
        float targetGain = 1.0f;
        float duration = 0.0f;
        float timer = 0.0f;
        bool active = false;
    };
    std::vector<AudioFade> fades;
};
