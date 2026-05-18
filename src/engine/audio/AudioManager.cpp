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

static ALCdevice* s_Device = nullptr;
static ALCcontext* s_Context = nullptr;
static int s_ActiveInstances = 0;
}

AudioManager::~AudioManager() {
    Shutdown();
}

bool AudioManager::Initialize() {
    if (initialized) {
        return true;
    }

    s_ActiveInstances++;

    if (!s_Device) {
        s_Device = alcOpenDevice(nullptr);
        if (!s_Device) {
            std::cout << "[Audio] Failed to open default OpenAL device\n";
            s_ActiveInstances--;
            return false;
        }
    }

    if (!s_Context) {
        s_Context = alcCreateContext(s_Device, nullptr);
        if (!s_Context) {
            std::cout << "[Audio] Failed to create OpenAL context\n";
            s_ActiveInstances--;
            return false;
        }
    }

    if (alcGetCurrentContext() != s_Context) {
        if (alcMakeContextCurrent(s_Context) == ALC_FALSE) {
            std::cout << "[Audio] Failed to activate OpenAL context\n";
            s_ActiveInstances--;
            return false;
        }
    }

    device = s_Device;
    context = s_Context;
    initialized = true;
    std::cout << "[Audio] OpenAL initialized (active instances: " << s_ActiveInstances << ")\n";
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

    s_ActiveInstances--;
    if (s_ActiveInstances <= 0) {
        alcMakeContextCurrent(nullptr);
        if (s_Context) {
            alcDestroyContext(s_Context);
            s_Context = nullptr;
        }
        if (s_Device) {
            alcCloseDevice(s_Device);
            s_Device = nullptr;
        }
        s_ActiveInstances = 0;
        std::cout << "[Audio] OpenAL completely shutdown\n";
    } else {
        if (alcGetCurrentContext() != s_Context && s_Context) {
            alcMakeContextCurrent(s_Context);
        }
        std::cout << "[Audio] Instance shutdown (active instances remaining: " << s_ActiveInstances << ")\n";
    }

    device = nullptr;
    context = nullptr;
    initialized = false;
}

ALuint AudioManager::DecodeWavToBuffer(const std::string& filePath) {
    SF_INFO info{};
    SNDFILE* sndFile = sf_open(filePath.c_str(), SFM_READ, &info);
    if (!sndFile) {
        std::cout << "[Audio] Failed to open sound file: " << filePath << "\n";
        return 0;
    }

    ALenum format = ResolveFormat(info.channels);
    if (format == AL_NONE) {
        std::cout << "[Audio] Unsupported channel count in " << filePath << ": " << info.channels << "\n";
        sf_close(sndFile);
        return 0;
    }

    std::vector<short> samples(static_cast<std::size_t>(info.frames) * static_cast<std::size_t>(info.channels));
    const sf_count_t readCount = sf_read_short(sndFile, samples.data(), static_cast<sf_count_t>(samples.size()));
    sf_close(sndFile);

    if (readCount <= 0) {
        std::cout << "[Audio] No samples read from: " << filePath << "\n";
        return 0;
    }

    ALuint buffer = 0;
    alGenBuffers(1, &buffer);
    if (buffer == 0) {
        std::cout << "[Audio] Failed to create buffer for: " << filePath << "\n";
        return 0;
    }

    alBufferData(
        buffer,
        format,
        samples.data(),
        static_cast<ALsizei>(readCount * static_cast<sf_count_t>(sizeof(short))),
        info.samplerate
    );

    return buffer;
}

bool AudioManager::LoadSound(const std::string& cueName, const std::string& filePath) {
    if (!initialized) {
        std::cout << "[Audio] LoadSound called before Initialize for cue: " << cueName << "\n";
        return false;
    }

    SoundInstance instance;
    instance.buffer = 0;
    instance.source = 0;
    instance.path = filePath;
    instance.loaded = false;

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
    return true;
}

bool AudioManager::HasSound(const std::string& cueName) const {
    return sounds.find(cueName) != sounds.end();
}

bool AudioManager::PlaySound(const std::string& cueName, float gain) {
    if (!initialized) {
        return false;
    }

    auto found = sounds.find(cueName);
    if (found == sounds.end()) {
        return false;
    }

    SoundInstance& instance = found->second;
    if (!instance.loaded) {
        instance.buffer = DecodeWavToBuffer(instance.path);
        if (instance.buffer == 0) {
            std::cout << "[Audio] Lazy load failed for cue: " << cueName << "\n";
            return false;
        }

        alGenSources(1, &instance.source);
        if (instance.source == 0) {
            std::cout << "[Audio] Failed to create source for lazy cue: " << cueName << "\n";
            alDeleteBuffers(1, &instance.buffer);
            instance.buffer = 0;
            return false;
        }

        alSourcei(instance.source, AL_BUFFER, static_cast<ALint>(instance.buffer));
        alSourcef(instance.source, AL_GAIN, 1.0f);
        alSourcef(instance.source, AL_PITCH, 1.0f);
        alSourcei(instance.source, AL_LOOPING, AL_FALSE);
        instance.loaded = true;
    }

    const float clampedGain = std::clamp(gain, 0.0f, 1.0f);
    ALuint source = instance.source;
    alSourcef(source, AL_GAIN, clampedGain);
    alSourceRewind(source);
    alSourcePlay(source);
    return true;
}

void AudioManager::StopSound(const std::string& cueName) {
    if (!initialized) {
        return;
    }
    auto found = sounds.find(cueName);
    if (found != sounds.end()) {
        SoundInstance& instance = found->second;
        if (instance.loaded && instance.source != 0) {
            alSourceStop(instance.source);
        }
    }
}

void AudioManager::SetMasterVolume(float volume) {
    if (initialized) {
        alListenerf(AL_GAIN, volume);
    }
}

