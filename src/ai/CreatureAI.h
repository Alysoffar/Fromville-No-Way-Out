#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "renderer/Model.h"
#include "renderer/SkeletalAnimator.h"

class Character;
class NavMesh;
class DayNightCycle;
class NPC;
class Renderer;

enum class CreatureState { DORMANT, PATROL, SEARCH, HUNT, WHISPER, RETREAT };

struct KnowledgeFlags {
	bool playerHasFirearms = false;
	bool playerUsedTunnels = false;
	bool playerRanLastEncounter = false;
	bool playerHasEscapeRoute = false;
};

struct Creature {
	int id = -1;
	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 velocity = glm::vec3(0.0f);
	float facingAngle = 0.0f;
	float health = 200.0f;
	CreatureState state = CreatureState::DORMANT;
	KnowledgeFlags knowledge;

	bool isInHumanForm = true;
	float transformProgress = 0.0f;
	float searchTimer = 0.0f;
	float whisperTimer = 0.0f;
	glm::vec3 lastKnownPlayerPos = glm::vec3(0.0f);
	float timeSinceLastSeen = 999.0f;
	int patrolWaypointIndex = 0;
	std::vector<glm::vec3> patrolPath;
	std::vector<glm::vec3> currentPath;
	int pathIndex = 0;
	NPC* whisperTarget = nullptr;
	float pathRefreshTimer = 0.0f;

	Model* humanModel = nullptr;
	Model* monsterModel = nullptr;
	SkeletalAnimator* humanAnimator = nullptr;
	SkeletalAnimator* monsterAnimator = nullptr;
};

class CreatureAI {
public:
	CreatureAI(NavMesh* navMesh, DayNightCycle* dayNight);
	void SpawnCreature(glm::vec3 position, std::vector<glm::vec3> patrolPath);
	void Update(float dt, std::vector<Character*>& players, std::vector<NPC*>& npcs);
	void DrawAll(Renderer& renderer);
	void NotifyPlayerAction(int playerId, std::string action, glm::vec3 atPos);
	float GetNearestCreatureDistance(glm::vec3 fromPos) const;
	Creature* GetNearestCreature(glm::vec3 fromPos);
	const std::vector<Creature>& GetCreatures() const { return creatures; }
	std::vector<Creature>& GetCreaturesMutable() { return creatures; }

private:
	std::vector<Creature> creatures;
	NavMesh* navMesh;
	DayNightCycle* dayNight;

	void UpdateDormant(Creature& c, float dt);
	void UpdatePatrol(Creature& c, float dt, std::vector<Character*>& players);
	void UpdateSearch(Creature& c, float dt, std::vector<Character*>& players);
	void UpdateHunt(Creature& c, float dt, std::vector<Character*>& players);
	void UpdateWhisper(Creature& c, float dt, std::vector<NPC*>& npcs);
	void UpdateRetreat(Creature& c, float dt);

	bool CanSeePlayer(const Creature& c, const Character* player) const;
	bool CanHearPlayer(const Creature& c, const Character* player) const;
	void RefreshPath(Creature& c, glm::vec3 goal);
	void MoveAlongPath(Creature& c, float speed, float dt);
	void TransitionTo(Creature& c, CreatureState newState);
	void HandleTransform(Creature& c, float dt, bool toMonster);
	void ApplyKnowledge(Creature& c, Character* target);
};

