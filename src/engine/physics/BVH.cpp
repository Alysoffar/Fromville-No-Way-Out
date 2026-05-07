#include "engine/physics/BVH.h"
#include <algorithm>
#include <stack>

void BVH::Build(std::vector<Triangle> triangles) {
    m_Triangles = std::move(triangles);
    m_TriangleIndices.resize(m_Triangles.size());
    for (int i = 0; i < (int)m_Triangles.size(); ++i) m_TriangleIndices[i] = i;

    m_Nodes.clear();
    m_Nodes.reserve(m_Triangles.size() * 2);
    
    // Root node
    m_Nodes.push_back({});
    m_Nodes[0].firstTriangle = 0;
    m_Nodes[0].triangleCount = (int)m_Triangles.size();
    
    UpdateNodeBounds(0);
    BuildRecursive(0, 0);
}

void BVH::UpdateNodeBounds(int nodeIndex) {
    BVHNode& node = m_Nodes[nodeIndex];
    node.bounds.min = glm::vec3(1e30f);
    node.bounds.max = glm::vec3(-1e30f);
    for (int i = 0; i < node.triangleCount; ++i) {
        node.bounds.Expand(m_Triangles[m_TriangleIndices[node.firstTriangle + i]].GetBounds());
    }
}

void BVH::BuildRecursive(int nodeIndex, int depth) {
    BVHNode& node = m_Nodes[nodeIndex];
    
    if (node.triangleCount <= 8 || depth > 20) return;

    // Split using midpoint on the largest axis
    glm::vec3 size = node.bounds.max - node.bounds.min;
    int axis = 0;
    if (size.y > size.x) axis = 1;
    if (size.z > size[axis]) axis = 2;
    
    float splitPos = node.bounds.min[axis] + size[axis] * 0.5f;

    // Partition triangles
    int i = node.firstTriangle;
    int j = i + node.triangleCount - 1;
    while (i <= j) {
        if (m_Triangles[m_TriangleIndices[i]].GetCenter()[axis] < splitPos) {
            i++;
        } else {
            std::swap(m_TriangleIndices[i], m_TriangleIndices[j]);
            j--;
        }
    }

    int leftCount = i - node.firstTriangle;
    if (leftCount == 0 || leftCount == node.triangleCount) return;

    int leftChildIdx = (int)m_Nodes.size();
    int rightChildIdx = leftChildIdx + 1;
    
    BVHNode leftNode, rightNode;
    leftNode.firstTriangle = node.firstTriangle;
    leftNode.triangleCount = leftCount;
    
    rightNode.firstTriangle = i;
    rightNode.triangleCount = node.triangleCount - leftCount;

    m_Nodes.push_back(leftNode);
    m_Nodes.push_back(rightNode);
    
    m_Nodes[nodeIndex].leftChild = leftChildIdx;
    m_Nodes[nodeIndex].rightChild = rightChildIdx;
    m_Nodes[nodeIndex].triangleCount = 0; // Internal node

    UpdateNodeBounds(leftChildIdx);
    UpdateNodeBounds(rightChildIdx);

    BuildRecursive(leftChildIdx, depth + 1);
    BuildRecursive(rightChildIdx, depth + 1);
}

bool BVH::Raycast(const glm::vec3& origin, const glm::vec3& dir, float maxDist, HitResult& out) const {
    if (m_Nodes.empty()) return false;

    Ray ray(origin, dir);
    float tMin, tMax;
    if (!RayAABB(ray, m_Nodes[0].bounds, tMin, tMax) || tMin > maxDist) return false;

    int stack[64];
    int stackPtr = 0;
    stack[stackPtr++] = 0;

    bool hit = false;
    float closestT = maxDist;

    while (stackPtr > 0) {
        int nodeIdx = stack[--stackPtr];
        const BVHNode& node = m_Nodes[nodeIdx];

        if (node.IsLeaf()) {
            for (int i = 0; i < node.triangleCount; ++i) {
                float t;
                glm::vec3 normal;
                const Triangle& tri = m_Triangles[m_TriangleIndices[node.firstTriangle + i]];
                if (RayTriangle(ray, tri.a, tri.b, tri.c, t, normal)) {
                    if (t < closestT) {
                        closestT = t;
                        out.point = origin + dir * t;
                        out.normal = normal;
                        out.t = t;
                        out.hit = true;
                        hit = true;
                    }
                }
            }
        } else {
            float tMinL, tMaxL, tMinR, tMaxR;
            bool hitL = RayAABB(ray, m_Nodes[node.leftChild].bounds, tMinL, tMaxL) && tMinL < closestT;
            bool hitR = RayAABB(ray, m_Nodes[node.rightChild].bounds, tMinR, tMaxR) && tMinR < closestT;

            if (hitL && hitR) {
                if (tMinL < tMinR) {
                    stack[stackPtr++] = node.rightChild;
                    stack[stackPtr++] = node.leftChild;
                } else {
                    stack[stackPtr++] = node.leftChild;
                    stack[stackPtr++] = node.rightChild;
                }
            } else if (hitL) {
                stack[stackPtr++] = node.leftChild;
            } else if (hitR) {
                stack[stackPtr++] = node.rightChild;
            }
        }
    }

    return hit;
}

bool BVH::OverlapAABB(const AABB& box) const {
    if (m_Nodes.empty()) return false;

    int stack[64];
    int stackPtr = 0;
    stack[stackPtr++] = 0;

    while (stackPtr > 0) {
        int nodeIdx = stack[--stackPtr];
        const BVHNode& node = m_Nodes[nodeIdx];

        if (!node.bounds.Intersects(box)) continue;

        if (node.IsLeaf()) {
            for (int i = 0; i < node.triangleCount; ++i) {
                const Triangle& tri = m_Triangles[m_TriangleIndices[node.firstTriangle + i]];
                if (AABBTriangle(box, tri.a, tri.b, tri.c)) return true;
            }
        } else {
            stack[stackPtr++] = node.rightChild;
            stack[stackPtr++] = node.leftChild;
        }
    }

    return false;
}

bool BVH::SweepAABB(const AABB& box, const glm::vec3& delta, HitResult& out) const {
    if (m_Nodes.empty()) return false;

    // Create a bounding box that covers the entire sweep
    AABB sweepBounds = box;
    sweepBounds.Expand(box.min + delta);
    sweepBounds.Expand(box.max + delta);

    int stack[64];
    int stackPtr = 0;
    stack[stackPtr++] = 0;

    bool hit = false;
    float closestT = 1.0f;

    while (stackPtr > 0) {
        int nodeIdx = stack[--stackPtr];
        const BVHNode& node = m_Nodes[nodeIdx];

        if (!node.bounds.Intersects(sweepBounds)) continue;

        if (node.IsLeaf()) {
            for (int i = 0; i < node.triangleCount; ++i) {
                float t;
                glm::vec3 normal;
                const Triangle& tri = m_Triangles[m_TriangleIndices[node.firstTriangle + i]];
                if (SweptAABBTriangle(box, delta, tri.a, tri.b, tri.c, t, normal)) {
                    if (t < closestT) {
                        closestT = t;
                        out.point = box.GetCenter() + delta * t; // Roughly contact point
                        out.normal = normal;
                        out.t = t;
                        out.hit = true;
                        hit = true;
                        
                        // Shrink sweep bounds to optimize further search
                        sweepBounds = box;
                        sweepBounds.Expand(box.min + delta * closestT);
                        sweepBounds.Expand(box.max + delta * closestT);
                    }
                }
            }
        } else {
            stack[stackPtr++] = node.rightChild;
            stack[stackPtr++] = node.leftChild;
        }
    }

    return hit;
}
