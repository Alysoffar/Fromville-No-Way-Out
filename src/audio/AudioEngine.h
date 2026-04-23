#pragma once

#include <string>
#include <unordered_map>

#include <AL/al.h>
#include <AL/alc.h>
#include <glm/glm.hpp>

class AudioEngine {
public:
	static AudioEngine& Get();
	bool Init();
	void Shutdown();
	void Update(glm::vec3 listenerPos, glm::vec3 listenerForward, glm::vec3 listenerUp);

	int LoadSound(const std::string& path);
	void UnloadSound(int bufferId);

	int PlaySound2D(int bufferId, float volume = 1.0f, float pitch = 1.0f, bool loop = false);
	int PlaySound3D(int bufferId,
					glm::vec3 pos,
					float volume = 1.0f,
					float pitch = 1.0f,
					bool loop = false,
					float refDist = 5.0f,
					float rolloff = 1.0f,
					float maxDist = 50.0f);
	void StopSource(int sourceId);
	void SetSourcePosition(int sourceId, glm::vec3 pos);
	void SetSourceVolume(int sourceId, float volume);
	void SetSourcePitch(int sourceId, float pitch);
	bool IsPlaying(int sourceId) const;
	void SetMasterVolume(float v);

	int PlayOneShotAt(const std::string& path, glm::vec3 pos, float vol = 1.0f);

	void PlayCreatureWhisper(glm::vec3 windowPos, float volumeMultiplier = 1.0f);
	void PlayBoydHeartbeat(float curseMeter);
	void SetAmbientLayer(const std::string& layerName, float volume);

private:
	AudioEngine() = default;
	~AudioEngine() = default;

	AudioEngine(const AudioEngine&) = delete;
	AudioEngine& operator=(const AudioEngine&) = delete;

	ALCdevice* device = nullptr;
	ALCcontext* context = nullptr;

	struct SoundBuffer {
		ALuint alBuffer = 0;
		std::string path;
		int refCount = 0;
	};

	std::unordered_map<int, SoundBuffer> buffers;
	std::unordered_map<int, ALuint> sources;
	std::unordered_map<std::string, int> pathToBufferId;
	int nextBufferId = 1;
	int nextSourceId = 1;
	float masterVolume = 1.0f;

	std::unordered_map<std::string, int> ambientSources;
	std::unordered_map<std::string, float> ambientCurrentVolumes;

	ALuint LoadOGG(const std::string& path);
	ALuint LoadWAV(const std::string& path);
	ALuint CreateSource(ALuint buffer, bool loop, float volume, float pitch);
	void PruneDeadSources();
};

