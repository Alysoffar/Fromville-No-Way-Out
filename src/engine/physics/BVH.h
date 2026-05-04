#pragma once

#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "engine/physics/Collision.h"
#include "engine/physics/Triangle.h"
#include "engine/physics/HitResult.h"

struct BVHNode {
    AABB bounds;
    int leftChild = -1;  // Index of left child in nodes vector
    int rightChild = -1; // Index of right child in nodes vector
    int firstTriangle = -1;
    int triangleCount = 0;

    bool IsLeaf() const { return triangleCount > 0; }
};

class BVH {
public:
    BVH() = default;
    
    void Build(std::vector<Triangle> triangles);
    
    bool Raycast(const glm::vec3& origin, const glm::vec3& dir, float maxDist, HitResult& out) const;
    bool OverlapAABB(const AABB& box) const;
    bool SweepAABB(const AABB& box, const glm::vec3& delta, HitResult& out) const;

private:
    void BuildRecursive(int nodeIndex, int depth);
    void UpdateNodeBounds(int nodeIndex);

    std::vector<Triangle> m_Triangles;
    std::vector<BVHNode> m_Nodes;
    std::vector<int> m_TriangleIndices;
};
