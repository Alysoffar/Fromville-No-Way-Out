#include "TalismanSystem.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "renderer/Renderer.h"

namespace {

void EmitEventTalismanPlaced(int nodeId) {
	std::printf("Event: TALISMAN_PLACED node=%d\n", nodeId);
}

void EmitEventTalismanRemoved(int nodeId) {
	std::printf("Event: TALISMAN_REMOVED node=%d\n", nodeId);
}

void EmitEventElderExpired(int nodeId) {
	std::printf("Event: ELDER_TALISMAN_EXPIRED node=%d\n", nodeId);
}

void NotifyNavMeshObstacleAdd(const TalismanNode& node) {
	(void)node;
	// Hook: map door/window entity to nav poly and block it.
}

void NotifyNavMeshObstacleRemove(const TalismanNode& node) {
	(void)node;
	// Hook: map door/window entity to nav poly and unblock it.
}

glm::vec3 JsonVec3(const nlohmann::json& j, const glm::vec3& fallback) {
	if (!j.is_array() || j.size() < 3) {
		return fallback;
	}
	return glm::vec3(j[0].get<float>(), j[1].get<float>(), j[2].get<float>());
}

} // namespace

TalismanSystem::TalismanSystem()
	: talismanShader(nullptr), talismanVAO(0) {}

void TalismanSystem::LoadNodesFromLevel(const nlohmann::json& levelData) {
	nodes.clear();

	const auto it = levelData.find("talismanNodes");
	if (it == levelData.end() || !it->is_array()) {
		return;
	}

	nodes.reserve(it->size());
	for (const auto& nodeJson : *it) {
		TalismanNode node;
		node.id = nodeJson.value("id", static_cast<int>(nodes.size()));
		node.position = JsonVec3(nodeJson.value("position", nlohmann::json::array()), glm::vec3(0.0f));
		node.normal = JsonVec3(nodeJson.value("normal", nlohmann::json::array()), glm::vec3(0.0f, 0.0f, 1.0f));
		node.hasTalisman = nodeJson.value("hasTalisman", false);
		node.isElder = nodeJson.value("isElder", false);
		node.elderGraceTimer = nodeJson.value("elderGraceTimer", 0.0f);
		node.attachedToEntity = nodeJson.value("attachedToEntity", std::string());
		node.renderModelId = static_cast<GLuint>(nodeJson.value("renderModelId", 0));
		nodes.push_back(std::move(node));
	}
}

void TalismanSystem::Update(float dt) {
	UpdateElderTalismans(dt);
}

void TalismanSystem::PlaceTalisman(int nodeId, bool isElder) {
	auto it = std::find_if(nodes.begin(), nodes.end(), [nodeId](const TalismanNode& n) { return n.id == nodeId; });
	if (it == nodes.end()) {
		return;
	}

	it->hasTalisman = true;
	it->isElder = isElder;
	it->elderGraceTimer = 0.0f;

	EmitEventTalismanPlaced(nodeId);
	NotifyNavMeshObstacleAdd(*it);
}

void TalismanSystem::RemoveTalisman(int nodeId) {
	auto it = std::find_if(nodes.begin(), nodes.end(), [nodeId](const TalismanNode& n) { return n.id == nodeId; });
	if (it == nodes.end()) {
		return;
	}

	it->hasTalisman = false;
	if (it->isElder) {
		it->elderGraceTimer = 10.0f;
	}

	EmitEventTalismanRemoved(nodeId);
	NotifyNavMeshObstacleRemove(*it);
}

bool TalismanSystem::IsNodeProtected(int nodeId) const {
	auto it = std::find_if(nodes.begin(), nodes.end(), [nodeId](const TalismanNode& n) { return n.id == nodeId; });
	if (it == nodes.end()) {
		return false;
	}
	return it->hasTalisman || (it->isElder && it->elderGraceTimer > 0.0f);
}

bool TalismanSystem::IsOpeningBreached(const std::string& entityName) const {
	for (const TalismanNode& n : nodes) {
		if (n.attachedToEntity == entityName) {
			return !IsNodeProtected(n.id);
		}
	}
	return true;
}

TalismanNode* TalismanSystem::GetNearestUnprotectedNode(glm::vec3 pos, float maxDist) {
	TalismanNode* best = nullptr;
	float bestDist = maxDist;
	for (TalismanNode& n : nodes) {
		if (IsNodeProtected(n.id)) {
			continue;
		}
		const float d = glm::distance(n.position, pos);
		if (d <= bestDist) {
			bestDist = d;
			best = &n;
		}
	}
	return best;
}

TalismanNode* TalismanSystem::GetNearestNode(glm::vec3 pos, float maxDist) {
	TalismanNode* best = nullptr;
	float bestDist = maxDist;
	for (TalismanNode& n : nodes) {
		const float d = glm::distance(n.position, pos);
		if (d <= bestDist) {
			bestDist = d;
			best = &n;
		}
	}
	return best;
}

void TalismanSystem::Draw(Renderer& renderer) {
	(void)renderer;
	for (const TalismanNode& n : nodes) {
		if (n.hasTalisman || (n.isElder && n.elderGraceTimer > 0.0f)) {
			DrawTalismanAt(n);
		}
	}
}

void TalismanSystem::UpdateElderTalismans(float dt) {
	for (TalismanNode& n : nodes) {
		if (n.isElder && !n.hasTalisman && n.elderGraceTimer > 0.0f) {
			n.elderGraceTimer = std::max(0.0f, n.elderGraceTimer - dt);
			if (n.elderGraceTimer == 0.0f) {
				EmitEventElderExpired(n.id);
			}
		}
	}
}

void TalismanSystem::DrawTalismanAt(const TalismanNode& node) {
	(void)node;
	// Hook: render talisman mesh at node.position facing node.normal.
}

