#include "engine/physics/CollisionWorld.h"
#include <tinyobjloader/tiny_obj_loader.h>
#include <iostream>
#include <algorithm>

bool CollisionWorld::LoadMap(const std::string& path) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str())) {
        std::cerr << "Failed to load collision map: " << err << std::endl;
        return false;
    }

    std::vector<Triangle> triangles;
    for (const auto& shape : shapes) {
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            if (shape.mesh.num_face_vertices[f] != 3) continue; // Only triangles

            Triangle tri;
            for (size_t v = 0; v < 3; v++) {
                tinyobj::index_t idx = shape.mesh.indices[f * 3 + v];
                tri.a = v == 0 ? glm::vec3(attrib.vertices[3 * idx.vertex_index + 0], attrib.vertices[3 * idx.vertex_index + 1], attrib.vertices[3 * idx.vertex_index + 2]) : tri.a;
                tri.b = v == 1 ? glm::vec3(attrib.vertices[3 * idx.vertex_index + 0], attrib.vertices[3 * idx.vertex_index + 1], attrib.vertices[3 * idx.vertex_index + 2]) : tri.b;
                tri.c = v == 2 ? glm::vec3(attrib.vertices[3 * idx.vertex_index + 0], attrib.vertices[3 * idx.vertex_index + 1], attrib.vertices[3 * idx.vertex_index + 2]) : tri.c;
            }
            triangles.push_back(tri);
        }
    }

    m_MapBVH.Build(std::move(triangles));
    return true;
}

bool CollisionWorld::RaycastMap(glm::vec3 origin, glm::vec3 dir, float maxDist, HitResult& out) const {
    return m_MapBVH.Raycast(origin, dir, maxDist, out);
}

bool CollisionWorld::SweepAABB(const AABB& box, glm::vec3 delta, HitResult& out) const {
    return m_MapBVH.SweepAABB(box, delta, out);
}

bool CollisionWorld::OverlapAABB(const AABB& box) const {
    return m_MapBVH.OverlapAABB(box);
}

glm::vec3 CollisionWorld::ResolveMovement(glm::vec3 position, glm::vec3 velocity, AABB localBox, float deltaTime) {
    glm::vec3 currentPos = position;
    glm::vec3 currentVel = velocity * deltaTime;
    
    for (int i = 0; i < 3; ++i) { // 3 iterations for corners
        if (glm::length(currentVel) < 0.0001f) break;

        AABB worldBox = localBox;
        worldBox.min += currentPos;
        worldBox.max += currentPos;

        HitResult hit;
        if (SweepAABB(worldBox, currentVel, hit)) {
            // Move up to the point of impact (with a tiny margin)
            float moveT = std::max(0.0f, hit.t - 0.001f);
            currentPos += currentVel * moveT;
            
            // Project remaining velocity along the collision normal (sliding)
            glm::vec3 remainingVel = currentVel * (1.0f - moveT);
            currentVel = remainingVel - glm::dot(remainingVel, hit.normal) * hit.normal;
        } else {
            // No collision, move full distance and break
            currentPos += currentVel;
            break;
        }
    }
    
    return currentPos;
}

bool CollisionWorld::IsGrounded(const AABB& box, float skinWidth) const {
    HitResult hit;
    // Small downward sweep from the bottom face
    return SweepAABB(box, glm::vec3(0.0f, -skinWidth, 0.0f), hit);
}
