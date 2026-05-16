#include "engine/ui/TransitionManager.h"

#include "engine/audio/AudioManager.h"

#include <algorithm>

TransitionManager& TransitionManager::Instance() {
    static TransitionManager inst;
    return inst;
}

TransitionManager::TransitionManager() = default;

void TransitionManager::Update(float dt) {
    // apply pending audio fades
    if (audio && !fades.empty()) {
        for (auto it = fades.begin(); it != fades.end();) {
            it->timer += dt;
            float t = it->duration <= 0.0f ? 1.0f : (it->timer / it->duration);
            if (t > 1.0f) t = 1.0f;
            const float gain = it->startGain + (it->targetGain - it->startGain) * t;
            audio->SetCueGain(it->cue, gain);
            if (t >= 1.0f) {
                it = fades.erase(it);
            } else {
                ++it;
            }
        }
    }

    if (!fading) return;
    fadeTimer += dt;
    if (fadeDuration <= 0.0f) {
        currentAlpha = fadeIn ? 0.0f : 1.0f;
        fading = false;
        if (completeCb) completeCb();
        return;
    }

    float t = fadeTimer / fadeDuration;
    if (t >= 1.0f) {
        t = 1.0f;
        fading = false;
        if (completeCb) completeCb();
    }

    currentAlpha = fadeIn ? (1.0f - t) : t;
}

void TransitionManager::StartFade(float duration, bool in, Callback onComplete) {
    fading = true;
    fadeDuration = duration;
    fadeTimer = 0.0f;
    fadeIn = in;
    completeCb = std::move(onComplete);
    currentAlpha = fadeIn ? 1.0f : 0.0f;
}

void TransitionManager::SetAudioManager(AudioManager* a) {
    audio = a;
}

void TransitionManager::StartAudioFade(const std::string& cueName, float targetGain, float duration) {
    if (!audio) return;
    // capture current gain (best effort)
    AudioFade fade;
    fade.cue = cueName;
    fade.duration = duration;
    fade.timer = 0.0f;
    fade.active = true;
    // No easy way to read current gain; assume 1.0
    fade.startGain = 1.0f;
    fade.targetGain = targetGain;
    fades.push_back(std::move(fade));
}

void TransitionManager::PlayStinger(const std::string& cueName, float gain) {
    if (!audio) return;
    audio->PlayOneShot(cueName, gain);
}
