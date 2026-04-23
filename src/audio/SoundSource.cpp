#include "SoundSource.h"

#include <utility>

#include "AudioEngine.h"

// ---------------------------------------------------------------------------
// Construction helpers
// ---------------------------------------------------------------------------

SoundSource::SoundSource(int sourceId)
    : sourceId_(sourceId) {}

SoundSource::~SoundSource() {
    Release();
}

SoundSource::SoundSource(SoundSource&& other) noexcept
    : sourceId_(other.sourceId_) {
    other.sourceId_ = -1;
}

SoundSource& SoundSource::operator=(SoundSource&& other) noexcept {
    if (this != &other) {
        Release();
        sourceId_       = other.sourceId_;
        other.sourceId_ = -1;
    }
    return *this;
}

// ---------------------------------------------------------------------------
// Internal release
// ---------------------------------------------------------------------------

void SoundSource::Release() {
    if (sourceId_ >= 0) {
        AudioEngine::Get().StopSource(sourceId_);
        sourceId_ = -1;
    }
}

// ---------------------------------------------------------------------------
// Convenience play helpers
// ---------------------------------------------------------------------------

bool SoundSource::Play2D(const std::string& path,
                         float volume,
                         float pitch,
                         bool  loop) {
    Release(); // stop any currently playing source first

    AudioEngine& ae = AudioEngine::Get();
    const int bufferId = ae.LoadSound(path);
    if (bufferId < 0) {
        return false;
    }

    const int id = ae.PlaySound2D(bufferId, volume, pitch, loop);
    if (id < 0) {
        return false;
    }

    sourceId_ = id;
    return true;
}

bool SoundSource::Play3D(const std::string& path,
                         glm::vec3 pos,
                         float volume,
                         float pitch,
                         bool  loop,
                         float refDist,
                         float rolloff,
                         float maxDist) {
    Release(); // stop any currently playing source first

    AudioEngine& ae = AudioEngine::Get();
    const int bufferId = ae.LoadSound(path);
    if (bufferId < 0) {
        return false;
    }

    const int id = ae.PlaySound3D(bufferId, pos, volume, pitch, loop,
                                   refDist, rolloff, maxDist);
    if (id < 0) {
        return false;
    }

    sourceId_ = id;
    return true;
}

// ---------------------------------------------------------------------------
// Runtime control
// ---------------------------------------------------------------------------

void SoundSource::Stop() {
    Release();
}

void SoundSource::SetPosition(glm::vec3 pos) {
    if (sourceId_ >= 0) {
        AudioEngine::Get().SetSourcePosition(sourceId_, pos);
    }
}

void SoundSource::SetVolume(float volume) {
    if (sourceId_ >= 0) {
        AudioEngine::Get().SetSourceVolume(sourceId_, volume);
    }
}

void SoundSource::SetPitch(float pitch) {
    if (sourceId_ >= 0) {
        AudioEngine::Get().SetSourcePitch(sourceId_, pitch);
    }
}

// ---------------------------------------------------------------------------
// Query
// ---------------------------------------------------------------------------

bool SoundSource::IsPlaying() const {
    if (sourceId_ < 0) {
        return false;
    }
    return AudioEngine::Get().IsPlaying(sourceId_);
}
