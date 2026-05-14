#pragma once

#include <string>
#include <unordered_map>

#include <AL/al.h>
#include <AL/alc.h>

class AudioManager {
public:
    AudioManager() = default;
    ~AudioManager();

    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    bool Initialize();
    void Shutdown();

    bool LoadSound(const std::string& cueName, const std::string& filePath);
    bool HasSound(const std::string& cueName) const;
    bool PlaySound(const std::string& cueName, float gain = 1.0f);

private:
    struct SoundInstance {
        ALuint buffer = 0;
        ALuint source = 0;
        std::string path;
    };

    std::unordered_map<std::string, SoundInstance> sounds;
    ALCdevice* device = nullptr;
    ALCcontext* context = nullptr;
    bool initialized = false;
};
