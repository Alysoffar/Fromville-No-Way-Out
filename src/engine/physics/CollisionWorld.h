#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "engine/physics/BVH.h"
#include "engine/physics/HitResult.h"
#include "engine/physics/Collision.h"
#include "engine/renderer/Mesh.h"

class CollisionWorld {
public:
    CollisionWorld() = default;
    
    bool LoadMap(const std::string& path);
    void AddTriangles(const std::vector<Triangle>& tris);
    void AddTrianglesFromMesh(const std::vector<MeshVertex>& vertices, const std::vector<unsigned int>& indices, const glm::mat4& transform, bool filterGround = false, bool filterLeaves = false, float treeMaxY = 0.0f);
    void AddAABB(const AABB& box, const glm::mat4& transform);
    void BuildBVH();
    void DeduplicateTriangles();
    
    bool RaycastMap(glm::vec3 origin, glm::vec3 dir, float maxDist, HitResult& out) const;
    bool SweepAABB(const AABB& box, glm::vec3 delta, HitResult& out) const;
    bool OverlapAABB(const AABB& box) const;
    bool IsPositionOccupied(const glm::vec3& position, float checkRadius = 0.5f) const;
    
    // Collision Response
    glm::vec3 ResolveMovement(glm::vec3 position, glm::vec3 velocity, AABB localBox, float deltaTime);
    
    // Ground Detection
    bool IsGrounded(const AABB& box, float skinWidth) const;

private:
    std::vector<Triangle> m_AllTriangles;
    BVH m_MapBVH;
    bool m_hasFlatGround = true;
    float m_groundY = 0.0f;
};
