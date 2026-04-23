#pragma once

#include <string>
#include <unordered_set>
#include <vector>

#include <glm/glm.hpp>

class DebugRenderer;

struct NavPoly {
	std::vector<glm::vec3> vertices;
	glm::vec3 centroid = glm::vec3(0.0f);
	std::vector<int> adjacentPolyIds;
};

class NavMesh {
public:
	bool LoadFromJSON(const std::string& path);
	std::vector<glm::vec3> FindPath(glm::vec3 start, glm::vec3 goal);
	bool IsWalkable(glm::vec3 point) const;
	int GetPolyAt(glm::vec3 point) const;
	void AddDynamicObstacle(int polyId);
	void RemoveDynamicObstacle(int polyId);
	void DebugDraw(DebugRenderer& dbg) const;

private:
	std::vector<NavPoly> polys;
	std::unordered_set<int> blockedPolys;

	std::vector<glm::vec3> AStar(int startPoly, int goalPoly);
	std::vector<glm::vec3> FunnelPath(const std::vector<int>& polyPath, glm::vec3 start, glm::vec3 goal);
	float Heuristic(int polyA, int polyB) const;
};

