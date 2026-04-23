#pragma once

#include <vector>

class AudioEngine;
class Character;
class Creature;
class NPC;
class TalismanSystem;
struct TalismanNode;

struct WhisperSession {
	int creatureId = -1;
	int talismanNodeId = -1;
	NPC* targetNPC = nullptr;
	float timer = 0.0f;
	float npcResistTimer = 15.0f;
	bool playerInterrupted = false;
};

class WhistleSystem {
public:
	WhistleSystem(TalismanSystem* talism, AudioEngine* audio);
	void Update(float dt, std::vector<Creature*>& creatures, std::vector<NPC*>& npcs, std::vector<Character*>& players);
	void OnPlayerInterrupts(NPC* npc);
	bool IsWhisperingAt(int talismanNodeId) const;

private:
	TalismanSystem* talismanSystem;
	AudioEngine* audioEngine;
	std::vector<WhisperSession> activeSessions;
	float whisperAudioInterval = 3.0f;

	void StartSession(Creature* c, int nodeId, NPC* npc);
	void UpdateSession(WhisperSession& session, float dt, std::vector<Character*>& players);
	void EndSession(WhisperSession& session, bool npcOpenedDoor);
	NPC* FindVulnerableNPC(const TalismanNode& node, std::vector<NPC*>& npcs);

	std::vector<Creature*>* creaturesRef = nullptr;
};

