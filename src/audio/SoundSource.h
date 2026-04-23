#pragma once

#include <string>

#include <glm/glm.hpp>

// ---------------------------------------------------------------------------
// SoundSource
//
// A lightweight RAII handle that wraps one source ID owned by AudioEngine.
// Destroying a SoundSource automatically stops and releases the underlying
// OpenAL source through AudioEngine.  Non-copyable; move-only.
//
// Typical use:
//   SoundSource footstep;
//   footstep.Play2D("assets/audio/footstep.ogg", 0.8f, 1.0f, true);
//
//   // or bind to an existing sourceId returned by AudioEngine directly:
//   SoundSource sfx(AudioEngine::Get().PlaySound3D(...));
// ---------------------------------------------------------------------------
class SoundSource {
public:
    // -----------------------------------------------------------------------
    // Construction / destruction
    // -----------------------------------------------------------------------

    /// Default – source starts invalid (no sound playing).
    SoundSource() = default;

    /// Adopt an existing sourceId already tracked by AudioEngine.
    explicit SoundSource(int sourceId);

    /// Stop + release on destruction.
    ~SoundSource();

    // Non-copyable
    SoundSource(const SoundSource&)            = delete;
    SoundSource& operator=(const SoundSource&) = delete;

    // Move-able
    SoundSource(SoundSource&& other) noexcept;
    SoundSource& operator=(SoundSource&& other) noexcept;

    // -----------------------------------------------------------------------
    // Convenience play helpers (load-and-play in one call)
    // -----------------------------------------------------------------------

    /// Load (or reuse) the file and start a 2-D (non-positional) source.
    /// Returns true on success.
    bool Play2D(const std::string& path,
                float volume = 1.0f,
                float pitch  = 1.0f,
                bool  loop   = false);

    /// Load (or reuse) the file and start a 3-D positional source.
    /// Returns true on success.
    bool Play3D(const std::string& path,
                glm::vec3 pos,
                float volume  = 1.0f,
                float pitch   = 1.0f,
                bool  loop    = false,
                float refDist = 5.0f,
                float rolloff = 1.0f,
                float maxDist = 50.0f);

    // -----------------------------------------------------------------------
    // Runtime control
    // -----------------------------------------------------------------------

    void Stop();
    void SetPosition(glm::vec3 pos);
    void SetVolume(float volume);
    void SetPitch(float pitch);

    // -----------------------------------------------------------------------
    // Query
    // -----------------------------------------------------------------------

    bool IsValid()   const { return sourceId_ >= 0; }
    bool IsPlaying() const;
    int  GetSourceId() const { return sourceId_; }

private:
    void Release();

    int sourceId_ = -1;
};
