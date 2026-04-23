#pragma once

#include <string>
#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

class Renderer;
class Shader;

struct TalismanNode {
	int id = -1;
	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 normal = glm::vec3(0.0f, 0.0f, 1.0f);
	bool hasTalisman = false;
	bool isElder = false;
	float elderGraceTimer = 0.0f;
	std::string attachedToEntity;
	GLuint renderModelId = 0;
};

class TalismanSystem {
public:
	TalismanSystem();
	void LoadNodesFromLevel(const nlohmann::json& levelData);
	void Update(float dt);
	void PlaceTalisman(int nodeId, bool isElder = false);
	void RemoveTalisman(int nodeId);
	bool IsNodeProtected(int nodeId) const;
	bool IsOpeningBreached(const std::string& entityName) const;
	TalismanNode* GetNearestUnprotectedNode(glm::vec3 pos, float maxDist = 2.0f);
	TalismanNode* GetNearestNode(glm::vec3 pos, float maxDist = 2.0f);
	void Draw(Renderer& renderer);
	const std::vector<TalismanNode>& GetAllNodes() const { return nodes; }

private:
	std::vector<TalismanNode> nodes;
	Shader* talismanShader;
	GLuint talismanVAO;

	void UpdateElderTalismans(float dt);
	void DrawTalismanAt(const TalismanNode& node);
};

