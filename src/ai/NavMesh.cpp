#include "NavMesh.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <fstream>
#include <limits>
#include <queue>
#include <unordered_map>

#include <nlohmann/json.hpp>

namespace {

glm::vec3 JsonToVec3(const nlohmann::json& arr) {
	if (!arr.is_array() || arr.size() < 3) {
		return glm::vec3(0.0f);
	}
	return glm::vec3(arr[0].get<float>(), arr[1].get<float>(), arr[2].get<float>());
}

glm::vec3 ComputeCentroid(const std::vector<glm::vec3>& vertices) {
	if (vertices.empty()) {
		return glm::vec3(0.0f);
	}

	glm::vec3 sum(0.0f);
	for (const glm::vec3& v : vertices) {
		sum += v;
	}
	return sum / static_cast<float>(vertices.size());
}

bool PointInPolyXZ(const glm::vec3& point, const std::vector<glm::vec3>& poly) {
	if (poly.size() < 3) {
		return false;
	}

	bool inside = false;
	const float px = point.x;
	const float pz = point.z;

	for (std::size_t i = 0, j = poly.size() - 1; i < poly.size(); j = i++) {
		const float xi = poly[i].x;
		const float zi = poly[i].z;
		const float xj = poly[j].x;
		const float zj = poly[j].z;

		const bool intersects = ((zi > pz) != (zj > pz)) &&
			(px < (xj - xi) * (pz - zi) / std::max(zj - zi, 0.00001f) + xi);
		if (intersects) {
			inside = !inside;
		}
	}
	return inside;
}

} // namespace

bool NavMesh::LoadFromJSON(const std::string& path) {
	std::ifstream file(path);
	if (!file.is_open()) {
		return false;
	}

	nlohmann::json root;
	try {
		file >> root;
	} catch (...) {
		return false;
	}

	const auto itPolys = root.find("polys");
	if (itPolys == root.end() || !itPolys->is_array()) {
		return false;
	}

	polys.clear();
	blockedPolys.clear();
	polys.reserve(itPolys->size());

	for (const auto& polyJson : *itPolys) {
		NavPoly poly;

		if (const auto itVerts = polyJson.find("vertices"); itVerts != polyJson.end() && itVerts->is_array()) {
			for (const auto& v : *itVerts) {
				poly.vertices.push_back(JsonToVec3(v));
			}
		}

		if (const auto itAdj = polyJson.find("adjacent"); itAdj != polyJson.end() && itAdj->is_array()) {
			for (const auto& id : *itAdj) {
				poly.adjacentPolyIds.push_back(id.get<int>());
			}
		}
		if (const auto itAdj2 = polyJson.find("adjacentPolyIds"); itAdj2 != polyJson.end() && itAdj2->is_array()) {
			for (const auto& id : *itAdj2) {
				poly.adjacentPolyIds.push_back(id.get<int>());
			}
		}

		poly.centroid = ComputeCentroid(poly.vertices);
		polys.push_back(std::move(poly));
	}

	return !polys.empty();
}

std::vector<glm::vec3> NavMesh::FindPath(glm::vec3 start, glm::vec3 goal) {
	const int startPoly = GetPolyAt(start);
	const int goalPoly = GetPolyAt(goal);
	if (startPoly < 0 || goalPoly < 0) {
		return {};
	}

	if (startPoly == goalPoly) {
		return {start, goal};
	}

	std::vector<glm::vec3> polyPath = AStar(startPoly, goalPoly);
	if (polyPath.empty()) {
		return {};
	}

	std::vector<int> polyIndices;
	polyIndices.reserve(polyPath.size());
	for (const glm::vec3& p : polyPath) {
		int best = -1;
		float bestDist = FLT_MAX;
		for (int i = 0; i < static_cast<int>(polys.size()); ++i) {
			const glm::vec3 delta = polys[i].centroid - p;
			const float d = glm::dot(delta, delta);
			if (d < bestDist) {
				bestDist = d;
				best = i;
			}
		}
		if (best >= 0) {
			polyIndices.push_back(best);
		}
	}

	return FunnelPath(polyIndices, start, goal);
}

bool NavMesh::IsWalkable(glm::vec3 point) const {
	const int polyId = GetPolyAt(point);
	return polyId >= 0 && blockedPolys.find(polyId) == blockedPolys.end();
}

int NavMesh::GetPolyAt(glm::vec3 point) const {
	for (int i = 0; i < static_cast<int>(polys.size()); ++i) {
		if (blockedPolys.find(i) != blockedPolys.end()) {
			continue;
		}
		if (PointInPolyXZ(point, polys[i].vertices)) {
			return i;
		}
	}
	return -1;
}

void NavMesh::AddDynamicObstacle(int polyId) {
	if (polyId >= 0 && polyId < static_cast<int>(polys.size())) {
		blockedPolys.insert(polyId);
	}
}

void NavMesh::RemoveDynamicObstacle(int polyId) {
	blockedPolys.erase(polyId);
}

void NavMesh::DebugDraw(DebugRenderer& dbg) const {
	(void)dbg;
	// Hook for navmesh debug visualization once DebugRenderer API is defined.
}

std::vector<glm::vec3> NavMesh::AStar(int startPoly, int goalPoly) {
	struct Node {
		int poly;
		float f;
		bool operator<(const Node& other) const { return f > other.f; }
	};

	std::priority_queue<Node> openSet;
	std::unordered_map<int, int> cameFrom;
	std::unordered_map<int, float> gScore;
	std::unordered_map<int, float> fScore;

	gScore[startPoly] = 0.0f;
	fScore[startPoly] = Heuristic(startPoly, goalPoly);
	openSet.push({startPoly, fScore[startPoly]});

	while (!openSet.empty()) {
		const int current = openSet.top().poly;
		openSet.pop();

		if (current == goalPoly) {
			std::vector<int> pathIds;
			int crawl = goalPoly;
			pathIds.push_back(crawl);
			while (cameFrom.find(crawl) != cameFrom.end()) {
				crawl = cameFrom[crawl];
				pathIds.push_back(crawl);
			}
			std::reverse(pathIds.begin(), pathIds.end());

			std::vector<glm::vec3> result;
			result.reserve(pathIds.size());
			for (int id : pathIds) {
				result.push_back(polys[id].centroid);
			}
			return result;
		}

		for (int neighbor : polys[current].adjacentPolyIds) {
			if (neighbor < 0 || neighbor >= static_cast<int>(polys.size())) {
				continue;
			}
			if (blockedPolys.find(neighbor) != blockedPolys.end()) {
				continue;
			}

			const float stepCost = glm::distance(polys[current].centroid, polys[neighbor].centroid);
			const float tentativeG = gScore[current] + stepCost;

			const auto gIt = gScore.find(neighbor);
			if (gIt == gScore.end() || tentativeG < gIt->second) {
				cameFrom[neighbor] = current;
				gScore[neighbor] = tentativeG;
				fScore[neighbor] = tentativeG + Heuristic(neighbor, goalPoly);
				openSet.push({neighbor, fScore[neighbor]});
			}
		}
	}

	return {};
}

std::vector<glm::vec3> NavMesh::FunnelPath(const std::vector<int>& polyPath, glm::vec3 start, glm::vec3 goal) {
	if (polyPath.empty()) {
		return {};
	}

	// Simplified funnel: use start -> intermediate centroids -> goal and remove tiny segments.
	std::vector<glm::vec3> out;
	out.push_back(start);

	for (int id : polyPath) {
		if (id < 0 || id >= static_cast<int>(polys.size())) {
			continue;
		}
		const glm::vec3 c = polys[id].centroid;
		if (glm::distance(out.back(), c) > 0.2f) {
			out.push_back(c);
		}
	}

	if (out.empty() || glm::distance(out.back(), goal) > 0.1f) {
		out.push_back(goal);
	}
	return out;
}

float NavMesh::Heuristic(int polyA, int polyB) const {
	if (polyA < 0 || polyA >= static_cast<int>(polys.size()) || polyB < 0 || polyB >= static_cast<int>(polys.size())) {
		return 0.0f;
	}
	return glm::distance(polys[polyA].centroid, polys[polyB].centroid);
}

