#include "AudioEngine.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <vector>

#include <sndfile.h>

namespace {

std::string ToLower(std::string s) {
	for (char& c : s) {
		c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	}
	return s;
}

std::string ExtensionOf(const std::string& path) {
	const std::size_t dot = path.find_last_of('.');
	if (dot == std::string::npos) {
		return std::string();
	}
	return ToLower(path.substr(dot + 1));
}

const char* AlErrorToString(ALenum error) {
	switch (error) {
	case AL_NO_ERROR:
		return "AL_NO_ERROR";
	case AL_INVALID_NAME:
		return "AL_INVALID_NAME";
	case AL_INVALID_ENUM:
		return "AL_INVALID_ENUM";
	case AL_INVALID_VALUE:
		return "AL_INVALID_VALUE";
	case AL_INVALID_OPERATION:
		return "AL_INVALID_OPERATION";
	case AL_OUT_OF_MEMORY:
		return "AL_OUT_OF_MEMORY";
	default:
		return "AL_UNKNOWN_ERROR";
	}
}

void LogAlErrorIfAny(const char* where) {
	const ALenum err = alGetError();
	if (err != AL_NO_ERROR) {
		std::printf("OpenAL error at %s: %s\n", where, AlErrorToString(err));
	}
}

} // namespace

AudioEngine& AudioEngine::Get() {
	static AudioEngine engine;
	return engine;
}

bool AudioEngine::Init() {
	if (context) {
		return true;
	}

	device = alcOpenDevice(nullptr);
	if (!device) {
		std::printf("AudioEngine: failed to open default OpenAL device\n");
		return false;
	}

	context = alcCreateContext(device, nullptr);
	if (!context) {
		std::printf("AudioEngine: failed to create OpenAL context\n");
		alcCloseDevice(device);
		device = nullptr;
		return false;
	}

	if (!alcMakeContextCurrent(context)) {
		std::printf("AudioEngine: failed to make OpenAL context current\n");
		alcDestroyContext(context);
		context = nullptr;
		alcCloseDevice(device);
		device = nullptr;
		return false;
	}

	alListener3f(AL_POSITION, 0.0f, 0.0f, 0.0f);
	alListenerf(AL_GAIN, 1.0f);
	alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);

	const ALCchar* deviceName = alcGetString(device, ALC_DEVICE_SPECIFIER);
	std::printf("AudioEngine: OpenAL device = %s\n", deviceName ? deviceName : "<unknown>");

	LogAlErrorIfAny("AudioEngine::Init");
	return true;
}

void AudioEngine::Shutdown() {
	for (auto& kv : sources) {
		ALuint src = kv.second;
		alSourceStop(src);
		alDeleteSources(1, &src);
	}
	sources.clear();

	for (auto& kv : buffers) {
		ALuint buf = kv.second.alBuffer;
		alDeleteBuffers(1, &buf);
	}
	buffers.clear();
	pathToBufferId.clear();
	ambientSources.clear();
	ambientCurrentVolumes.clear();

	if (context) {
		alcMakeContextCurrent(nullptr);
		alcDestroyContext(context);
		context = nullptr;
	}
	if (device) {
		alcCloseDevice(device);
		device = nullptr;
	}
}

void AudioEngine::Update(glm::vec3 listenerPos, glm::vec3 listenerForward, glm::vec3 listenerUp) {
	if (!context) {
		return;
	}

	alListener3f(AL_POSITION, listenerPos.x, listenerPos.y, listenerPos.z);
	const ALfloat orientation[] = {
		listenerForward.x, listenerForward.y, listenerForward.z,
		listenerUp.x, listenerUp.y, listenerUp.z
	};
	alListenerfv(AL_ORIENTATION, orientation);

	PruneDeadSources();
}

int AudioEngine::LoadSound(const std::string& path) {
	if (path.empty()) {
		return -1;
	}

	auto foundPath = pathToBufferId.find(path);
	if (foundPath != pathToBufferId.end()) {
		auto it = buffers.find(foundPath->second);
		if (it != buffers.end()) {
			it->second.refCount += 1;
			return it->first;
		}
	}

	ALuint alBuf = 0;
	const std::string ext = ExtensionOf(path);
	if (ext == "ogg") {
		alBuf = LoadOGG(path);
	} else if (ext == "wav") {
		alBuf = LoadWAV(path);
	} else {
		alBuf = LoadOGG(path);
	}

	if (alBuf == 0) {
		std::printf("AudioEngine: failed to load sound '%s'\n", path.c_str());
		return -1;
	}

	const int id = nextBufferId++;
	SoundBuffer sb;
	sb.alBuffer = alBuf;
	sb.path = path;
	sb.refCount = 1;
	buffers.emplace(id, sb);
	pathToBufferId[path] = id;
	return id;
}

void AudioEngine::UnloadSound(int bufferId) {
	auto it = buffers.find(bufferId);
	if (it == buffers.end()) {
		return;
	}

	it->second.refCount -= 1;
	if (it->second.refCount > 0) {
		return;
	}

	for (auto srcIt = sources.begin(); srcIt != sources.end();) {
		ALint currentBuffer = 0;
		alGetSourcei(srcIt->second, AL_BUFFER, &currentBuffer);
		if (static_cast<ALuint>(currentBuffer) == it->second.alBuffer) {
			alSourceStop(srcIt->second);
			alDeleteSources(1, &srcIt->second);
			srcIt = sources.erase(srcIt);
		} else {
			++srcIt;
		}
	}

	pathToBufferId.erase(it->second.path);
	alDeleteBuffers(1, &it->second.alBuffer);
	buffers.erase(it);
}

int AudioEngine::PlaySound2D(int bufferId, float volume, float pitch, bool loop) {
	auto it = buffers.find(bufferId);
	if (it == buffers.end()) {
		return -1;
	}

	ALuint src = CreateSource(it->second.alBuffer, loop, volume, pitch);
	if (src == 0) {
		return -1;
	}

	alSourcei(src, AL_SOURCE_RELATIVE, AL_TRUE);
	alSource3f(src, AL_POSITION, 0.0f, 0.0f, 0.0f);
	alSourcePlay(src);

	const int sourceId = nextSourceId++;
	sources[sourceId] = src;
	return sourceId;
}

int AudioEngine::PlaySound3D(int bufferId,
							 glm::vec3 pos,
							 float volume,
							 float pitch,
							 bool loop,
							 float refDist,
							 float rolloff,
							 float maxDist) {
	auto it = buffers.find(bufferId);
	if (it == buffers.end()) {
		return -1;
	}

	ALuint src = CreateSource(it->second.alBuffer, loop, volume, pitch);
	if (src == 0) {
		return -1;
	}

	alSource3f(src, AL_POSITION, pos.x, pos.y, pos.z);
	alSourcef(src, AL_REFERENCE_DISTANCE, refDist);
	alSourcef(src, AL_ROLLOFF_FACTOR, rolloff);
	alSourcef(src, AL_MAX_DISTANCE, maxDist);
	alSourcei(src, AL_SOURCE_RELATIVE, AL_FALSE);
	alSourcePlay(src);

	const int sourceId = nextSourceId++;
	sources[sourceId] = src;
	return sourceId;
}

void AudioEngine::StopSource(int sourceId) {
	auto it = sources.find(sourceId);
	if (it == sources.end()) {
		return;
	}

	alSourceStop(it->second);
	alDeleteSources(1, &it->second);
	sources.erase(it);
}

void AudioEngine::SetSourcePosition(int sourceId, glm::vec3 pos) {
	auto it = sources.find(sourceId);
	if (it == sources.end()) {
		return;
	}
	alSource3f(it->second, AL_POSITION, pos.x, pos.y, pos.z);
}

void AudioEngine::SetSourceVolume(int sourceId, float volume) {
	auto it = sources.find(sourceId);
	if (it == sources.end()) {
		return;
	}
	const float v = std::clamp(volume, 0.0f, 1.0f);
	alSourcef(it->second, AL_GAIN, v * masterVolume);
}

void AudioEngine::SetSourcePitch(int sourceId, float pitch) {
	auto it = sources.find(sourceId);
	if (it == sources.end()) {
		return;
	}
	const float p = std::max(0.01f, pitch);
	alSourcef(it->second, AL_PITCH, p);
}

bool AudioEngine::IsPlaying(int sourceId) const {
	auto it = sources.find(sourceId);
	if (it == sources.end()) {
		return false;
	}

	ALint state = AL_STOPPED;
	alGetSourcei(it->second, AL_SOURCE_STATE, &state);
	return state == AL_PLAYING;
}

void AudioEngine::SetMasterVolume(float v) {
	masterVolume = std::clamp(v, 0.0f, 1.0f);
	alListenerf(AL_GAIN, masterVolume);
}

int AudioEngine::PlayOneShotAt(const std::string& path, glm::vec3 pos, float vol) {
	const int bufferId = LoadSound(path);
	if (bufferId < 0) {
		return -1;
	}
	return PlaySound3D(bufferId, pos, vol, 1.0f, false);
}

void AudioEngine::PlayCreatureWhisper(glm::vec3 windowPos, float volumeMultiplier) {
	const int buf = LoadSound("assets/audio/creature_whisper.ogg");
	if (buf < 0) {
		return;
	}
	PlaySound3D(buf, windowPos, 0.5f * volumeMultiplier, 1.0f, false, 2.0f, 2.0f, 20.0f);
}

void AudioEngine::PlayBoydHeartbeat(float curseMeter) {
	static int srcId = -1;

	const float cm = std::clamp(curseMeter, 0.0f, 1.0f);
	const float pitch = 0.8f + cm * 0.6f;
	const float volume = (cm > 0.4f) ? (cm - 0.4f) / 0.6f : 0.0f;

	if (srcId == -1 || !IsPlaying(srcId)) {
		const int buf = LoadSound("assets/audio/heartbeat.ogg");
		if (buf < 0) {
			return;
		}
		srcId = PlaySound2D(buf, volume, pitch, true);
	} else {
		SetSourceVolume(srcId, volume);
		SetSourcePitch(srcId, pitch);
	}
}

void AudioEngine::SetAmbientLayer(const std::string& layerName, float volume) {
	static const std::unordered_map<std::string, std::string> layerToPath = {
		{"forest_day", "assets/audio/forest_day.ogg"},
		{"forest_night", "assets/audio/forest_night.ogg"},
		{"tunnel_ambient", "assets/audio/tunnel_ambient.ogg"},
	};

	const float targetVol = std::clamp(volume, 0.0f, 1.0f);
	for (const auto& kv : layerToPath) {
		const std::string& layer = kv.first;
		const std::string& path = kv.second;

		if (ambientSources.find(layer) == ambientSources.end()) {
			const int buf = LoadSound(path);
			if (buf >= 0) {
				const int src = PlaySound2D(buf, 0.0f, 1.0f, true);
				if (src >= 0) {
					ambientSources[layer] = src;
					ambientCurrentVolumes[layer] = 0.0f;
				}
			}
		}

		auto srcIt = ambientSources.find(layer);
		if (srcIt == ambientSources.end()) {
			continue;
		}

		const float desired = (layer == layerName) ? targetVol : 0.0f;
		float& current = ambientCurrentVolumes[layer];
		const float step = 0.05f;
		if (current < desired) {
			current = std::min(desired, current + step);
		} else if (current > desired) {
			current = std::max(desired, current - step);
		}
		SetSourceVolume(srcIt->second, current);
	}
}

ALuint AudioEngine::LoadOGG(const std::string& path) {
	SF_INFO info{};
	SNDFILE* sf = sf_open(path.c_str(), SFM_READ, &info);
	if (!sf) {
		std::printf("AudioEngine: sf_open failed for %s\n", path.c_str());
		return 0;
	}

	if (info.channels < 1 || info.channels > 2) {
		std::printf("AudioEngine: unsupported channel count %d in %s\n", info.channels, path.c_str());
		sf_close(sf);
		return 0;
	}

	std::vector<short> pcm(static_cast<std::size_t>(info.frames) * static_cast<std::size_t>(info.channels));
	const sf_count_t readFrames = sf_readf_short(sf, pcm.data(), info.frames);
	sf_close(sf);
	if (readFrames <= 0) {
		std::printf("AudioEngine: no frames read from %s\n", path.c_str());
		return 0;
	}

	ALenum format = (info.channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
	ALuint buf = 0;
	alGenBuffers(1, &buf);
	alBufferData(buf,
				 format,
				 pcm.data(),
				 static_cast<ALsizei>(readFrames * info.channels * static_cast<sf_count_t>(sizeof(short))),
				 info.samplerate);
	LogAlErrorIfAny("AudioEngine::LoadOGG");
	return buf;
}

ALuint AudioEngine::LoadWAV(const std::string& path) {
	return LoadOGG(path);
}

ALuint AudioEngine::CreateSource(ALuint buffer, bool loop, float volume, float pitch) {
	if (buffer == 0) {
		return 0;
	}

	ALuint src = 0;
	alGenSources(1, &src);
	if (src == 0) {
		return 0;
	}

	alSourcei(src, AL_BUFFER, static_cast<ALint>(buffer));
	alSourcei(src, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
	alSourcef(src, AL_GAIN, std::clamp(volume, 0.0f, 1.0f) * masterVolume);
	alSourcef(src, AL_PITCH, std::max(0.01f, pitch));
	return src;
}

void AudioEngine::PruneDeadSources() {
	for (auto it = sources.begin(); it != sources.end();) {
		ALint state = AL_STOPPED;
		ALint looping = AL_FALSE;
		alGetSourcei(it->second, AL_SOURCE_STATE, &state);
		alGetSourcei(it->second, AL_LOOPING, &looping);

		if (state == AL_STOPPED && looping == AL_FALSE) {
			alDeleteSources(1, &it->second);
			it = sources.erase(it);
		} else {
			++it;
		}
	}
}

