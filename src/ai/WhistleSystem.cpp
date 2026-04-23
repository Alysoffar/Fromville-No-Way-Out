#include "WhistleSystem.h"

#include <algorithm>
#include <cstdio>
#include <string>

#include "CreatureAI.h"
#include "characters/Character.h"
#include "world/TalismanSystem.h"

namespace {

const Character* FindSaraWithinRange(const glm::vec3& pos, const std::vector<Character*>& players, float rangeMeters) {
	const float r2 = rangeMeters * rangeMeters;
	for (const Character* player : players) {
		if (!player || !player->IsAlive()) {
			continue;
		}
		if (player->name != "Sara") {
			continue;
		}
		const glm::vec3 d = player->position - pos;
		if (glm::dot(d, d) <= r2) {
			return player;
		}
	}
	return nullptr;
}

void PlayWhisperAudio3D(AudioEngine* audioEngine, const glm::vec3& at, float volume) {
	(void)audioEngine;
	(void)at;
	std::printf("Audio: creature_whisper.ogg volume=%.2f\n", volume);
}

void EmitNpcOpenedDoorEvent(NPC* npc, int nodeId) {
	(void)npc;
	std::printf("Event: NPC_OPENED_DOOR node=%d\n", nodeId);
}

bool NpcIsAvailable(NPC* npc) {
	return npc != nullptr;
}

void NpcWalkToAndOpen(NPC* npc, const std::string& openingEntity) {
	(void)npc;
	(void)openingEntity;
	// Hook: npc->WalkToAndOpen(openingEntity)
}

const TalismanNode* FindNodeById(const TalismanSystem* talismanSystem, int nodeId) {
	if (!talismanSystem) {
		return nullptr;
	}
	const std::vector<TalismanNode>& nodes = talismanSystem->GetAllNodes();
	for (const TalismanNode& n : nodes) {
		if (n.id == nodeId) {
			return &n;
		}
	}
	return nullptr;
}

} // namespace

WhistleSystem::WhistleSystem(TalismanSystem* talism, AudioEngine* audio)
	: talismanSystem(talism), audioEngine(audio) {}

void WhistleSystem::Update(float dt, std::vector<Creature*>& creatures, std::vector<NPC*>& npcs, std::vector<Character*>& players) {
	creaturesRef = &creatures;

	for (Creature* c : creatures) {
		if (!c || c->state != CreatureState::WHISPER) {
			continue;
		}

		TalismanNode* node = talismanSystem ? talismanSystem->GetNearestNode(c->position, 6.0f) : nullptr;
		if (!node || !talismanSystem->IsNodeProtected(node->id) || IsWhisperingAt(node->id)) {
			continue;
		}

		NPC* vulnerable = FindVulnerableNPC(*node, npcs);
		if (vulnerable) {
			StartSession(c, node->id, vulnerable);
		}
	}

	std::vector<int> removeIndices;
	for (int i = 0; i < static_cast<int>(activeSessions.size()); ++i) {
		WhisperSession& session = activeSessions[i];
		const bool hadNpc = (session.targetNPC != nullptr);
		UpdateSession(session, dt, players);
		if (!hadNpc || session.targetNPC == nullptr) {
			removeIndices.push_back(i);
		}
	}

	std::sort(removeIndices.rbegin(), removeIndices.rend());
	for (int idx : removeIndices) {
		activeSessions.erase(activeSessions.begin() + idx);
	}
}

void WhistleSystem::OnPlayerInterrupts(NPC* npc) {
	for (WhisperSession& session : activeSessions) {
		if (session.targetNPC == npc) {
			session.playerInterrupted = true;
		}
	}
}

bool WhistleSystem::IsWhisperingAt(int talismanNodeId) const {
	for (const WhisperSession& s : activeSessions) {
		if (s.talismanNodeId == talismanNodeId) {
			return true;
		}
	}
	return false;
}

void WhistleSystem::StartSession(Creature* c, int nodeId, NPC* npc) {
	if (!c || !npc) {
		return;
	}
	WhisperSession session;
	session.creatureId = c->id;
	session.talismanNodeId = nodeId;
	session.targetNPC = npc;
	activeSessions.push_back(session);
}

void WhistleSystem::UpdateSession(WhisperSession& session, float dt, std::vector<Character*>& players) {
	if (!session.targetNPC || !talismanSystem) {
		session.targetNPC = nullptr;
		return;
	}

	const TalismanNode* node = FindNodeById(talismanSystem, session.talismanNodeId);
	if (!node) {
		session.targetNPC = nullptr;
		return;
	}

	const float prevTimer = session.timer;
	session.timer += dt;

	const int prevTick = static_cast<int>(prevTimer / whisperAudioInterval);
	const int currTick = static_cast<int>(session.timer / whisperAudioInterval);
	if (currTick > prevTick) {
		PlayWhisperAudio3D(audioEngine, node->position, 0.5f);
		if (FindSaraWithinRange(node->position, players, 12.0f)) {
			PlayWhisperAudio3D(audioEngine, node->position, 1.0f);
		}
	}

	session.npcResistTimer -= dt;

	if (session.playerInterrupted) {
		EndSession(session, false);
		return;
	}

	if (session.npcResistTimer <= 0.0f) {
		NpcWalkToAndOpen(session.targetNPC, node->attachedToEntity);
		EndSession(session, true);
	}
}

void WhistleSystem::EndSession(WhisperSession& session, bool npcOpenedDoor) {
	if (npcOpenedDoor && talismanSystem) {
		talismanSystem->RemoveTalisman(session.talismanNodeId);
		EmitNpcOpenedDoorEvent(session.targetNPC, session.talismanNodeId);
		if (creaturesRef) {
			for (Creature* c : *creaturesRef) {
				if (c && c->id == session.creatureId) {
					c->state = CreatureState::HUNT;
					break;
				}
			}
		}
	}

	session.targetNPC = nullptr;
}

NPC* WhistleSystem::FindVulnerableNPC(const TalismanNode& node, std::vector<NPC*>& npcs) {
	(void)node;
	for (NPC* npc : npcs) {
		if (NpcIsAvailable(npc)) {
			return npc;
		}
	}
	return nullptr;
}

