#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "engine/physics/BVH.h"
#include "engine/physics/HitResult.h"
#include "engine/physics/Collision.h"

class CollisionWorld {
public:
    CollisionWorld() = default;
    
    bool LoadMap(const std::string& path);
    
    bool RaycastMap(glm::vec3 origin, glm::vec3 dir, float maxDist, HitResult& out) const;
    bool SweepAABB(const AABB& box, glm::vec3 delta, HitResult& out) const;
    bool OverlapAABB(const AABB& box) const;
    
    // Collision Response
    glm::vec3 ResolveMovement(glm::vec3 position, glm::vec3 velocity, AABB localBox, float deltaTime);
    
    // Ground Detection
    bool IsGrounded(const AABB& box, float skinWidth) const;

private:
    BVH m_MapBVH;
};
