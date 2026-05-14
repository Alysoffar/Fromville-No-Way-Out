#include "engine/audio/AudioManager.h"

#include <algorithm>
#include <iostream>
#include <vector>

#include <sndfile.h>

namespace {
ALenum ResolveFormat(int channels) {
    if (channels == 1) {
        return AL_FORMAT_MONO16;
    }
    if (channels == 2) {
        return AL_FORMAT_STEREO16;
    }
    return AL_NONE;
}
}

AudioManager::~AudioManager() {
    Shutdown();
}

bool AudioManager::Initialize() {
    if (initialized) {
        return true;
    }

    device = alcOpenDevice(nullptr);
    if (!device) {
        std::cout << "[Audio] Failed to open default OpenAL device\n";
        return false;
    }

    context = alcCreateContext(device, nullptr);
    if (!context) {
        std::cout << "[Audio] Failed to create OpenAL context\n";
        alcCloseDevice(device);
        device = nullptr;
        return false;
    }

    if (alcMakeContextCurrent(context) == ALC_FALSE) {
        std::cout << "[Audio] Failed to activate OpenAL context\n";
        alcDestroyContext(context);
        context = nullptr;
        alcCloseDevice(device);
        device = nullptr;
        return false;
    }

    initialized = true;
    std::cout << "[Audio] OpenAL initialized\n";
    return true;
}

void AudioManager::Shutdown() {
    if (!initialized) {
        return;
    }

    for (auto& [cue, sound] : sounds) {
        if (sound.source != 0) {
            alSourceStop(sound.source);
            alDeleteSources(1, &sound.source);
            sound.source = 0;
        }
        if (sound.buffer != 0) {
            alDeleteBuffers(1, &sound.buffer);
            sound.buffer = 0;
        }
    }
    sounds.clear();

    alcMakeContextCurrent(nullptr);
    if (context) {
        alcDestroyContext(context);
        context = nullptr;
    }
    if (device) {
        alcCloseDevice(device);
        device = nullptr;
    }

    initialized = false;
}

bool AudioManager::LoadSound(const std::string& cueName, const std::string& filePath) {
    if (!initialized) {
        std::cout << "[Audio] LoadSound called before Initialize for cue: " << cueName << "\n";
        return false;
    }

    SF_INFO info{};
    SNDFILE* sndFile = sf_open(filePath.c_str(), SFM_READ, &info);
    if (!sndFile) {
        std::cout << "[Audio] Failed to open sound file: " << filePath << "\n";
        return false;
    }

    ALenum format = ResolveFormat(info.channels);
    if (format == AL_NONE) {
        std::cout << "[Audio] Unsupported channel count in " << filePath << ": " << info.channels << "\n";
        sf_close(sndFile);
        return false;
    }

    std::vector<short> samples(static_cast<std::size_t>(info.frames) * static_cast<std::size_t>(info.channels));
    const sf_count_t readCount = sf_read_short(sndFile, samples.data(), static_cast<sf_count_t>(samples.size()));
    sf_close(sndFile);

    if (readCount <= 0) {
        std::cout << "[Audio] No samples read from: " << filePath << "\n";
        return false;
    }

    SoundInstance instance;
    alGenBuffers(1, &instance.buffer);
    if (instance.buffer == 0) {
        std::cout << "[Audio] Failed to create buffer for cue: " << cueName << "\n";
        return false;
    }

    alBufferData(
        instance.buffer,
        format,
        samples.data(),
        static_cast<ALsizei>(readCount * static_cast<sf_count_t>(sizeof(short))),
        info.samplerate
    );

    alGenSources(1, &instance.source);
    if (instance.source == 0) {
        std::cout << "[Audio] Failed to create source for cue: " << cueName << "\n";
        alDeleteBuffers(1, &instance.buffer);
        instance.buffer = 0;
        return false;
    }

    alSourcei(instance.source, AL_BUFFER, static_cast<ALint>(instance.buffer));
    alSourcef(instance.source, AL_GAIN, 1.0f);
    alSourcef(instance.source, AL_PITCH, 1.0f);
    alSourcei(instance.source, AL_LOOPING, AL_FALSE);
    instance.path = filePath;

    auto existing = sounds.find(cueName);
    if (existing != sounds.end()) {
        if (existing->second.source != 0) {
            alSourceStop(existing->second.source);
            alDeleteSources(1, &existing->second.source);
        }
        if (existing->second.buffer != 0) {
            alDeleteBuffers(1, &existing->second.buffer);
        }
    }

    sounds[cueName] = instance;
    std::cout << "[Audio] Loaded cue '" << cueName << "' from " << filePath << "\n";
    return true;
}

bool AudioManager::HasSound(const std::string& cueName) const {
    return sounds.find(cueName) != sounds.end();
}

bool AudioManager::PlaySound(const std::string& cueName, float gain) {
    if (!initialized) {
        return false;
    }

    const auto found = sounds.find(cueName);
    if (found == sounds.end()) {
        return false;
    }

    const float clampedGain = std::clamp(gain, 0.0f, 1.0f);
    ALuint source = found->second.source;
    alSourcef(source, AL_GAIN, clampedGain);
    alSourceRewind(source);
    alSourcePlay(source);
    return true;
}
