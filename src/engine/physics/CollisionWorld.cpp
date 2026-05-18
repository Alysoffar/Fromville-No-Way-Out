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

    AddTriangles(triangles);
    return true;
}

void CollisionWorld::AddTriangles(const std::vector<Triangle>& tris) {
    m_AllTriangles.insert(m_AllTriangles.end(), tris.begin(), tris.end());
}

void CollisionWorld::AddTrianglesFromMesh(const std::vector<MeshVertex>& vertices, const std::vector<unsigned int>& indices, const glm::mat4& transform, bool filterGround, bool filterLeaves, float treeMaxY) {
    std::vector<Triangle> triangles;
    triangles.reserve(indices.size() / 3);
    for (size_t i = 0; i < indices.size(); i += 3) {
        
        float localMaxY = std::max(vertices[indices[i]].position.y, 
                                   std::max(vertices[indices[i+1]].position.y, vertices[indices[i+2]].position.y));
        
        if (filterLeaves && localMaxY > treeMaxY * 0.6f) {
            continue; // Skip the canopy/leaves
        }

        Triangle tri;
        tri.a = glm::vec3(transform * glm::vec4(vertices[indices[i]].position, 1.0f));
        tri.b = glm::vec3(transform * glm::vec4(vertices[indices[i+1]].position, 1.0f));
        tri.c = glm::vec3(transform * glm::vec4(vertices[indices[i+2]].position, 1.0f));
        
        // Skip triangles that are flat and very close to the world floor (Y=0.0)
        // Wait, the floor of the house is often Y < 0.2f. We can also check if the normal is pointing up.
        if (filterGround) {
            float maxY = std::max(tri.a.y, std::max(tri.b.y, tri.c.y));
            if (maxY < 0.2f) {
                // simple check for upward-facing normal to be sure it's a floor, but maxY < 0.2f is usually enough
                glm::vec3 edge1 = tri.b - tri.a;
                glm::vec3 edge2 = tri.c - tri.a;
                glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));
                if (normal.y > 0.8f || normal.y < -0.8f) {
                    continue;
                }
            }
        }
        
        triangles.push_back(tri);
    }
    m_AllTriangles.insert(m_AllTriangles.end(), triangles.begin(), triangles.end());
}

void CollisionWorld::AddAABB(const AABB& box, const glm::mat4& transform) {
    glm::vec3 c[8] = {
        {box.min.x, box.min.y, box.min.z}, {box.max.x, box.min.y, box.min.z},
        {box.max.x, box.max.y, box.min.z}, {box.min.x, box.max.y, box.min.z},
        {box.min.x, box.min.y, box.max.z}, {box.max.x, box.min.y, box.max.z},
        {box.max.x, box.max.y, box.max.z}, {box.min.x, box.max.y, box.max.z}
    };
    for(int i=0; i<8; ++i) c[i] = glm::vec3(transform * glm::vec4(c[i], 1.0f));

    int idx[] = {
        0,1,2, 0,2,3, 1,5,6, 1,6,2, 5,4,7, 5,7,6, 4,0,3, 4,3,7, 3,2,6, 3,6,7, 4,5,1, 4,1,0
    };
    std::vector<Triangle> triangles(12);
    for(int i=0; i<12; ++i) {
        triangles[i].a = c[idx[i*3]]; 
        triangles[i].b = c[idx[i*3+1]]; 
        triangles[i].c = c[idx[i*3+2]];
    }
    m_AllTriangles.insert(m_AllTriangles.end(), triangles.begin(), triangles.end());
}

void CollisionWorld::BuildBVH() {
    printf("CollisionWorld triangle count before BVH: %d\n", (int)m_AllTriangles.size());
    DeduplicateTriangles();
    m_MapBVH.Build(m_AllTriangles);
}

void CollisionWorld::DeduplicateTriangles() {
    std::vector<Triangle> uniqueTriangles;
    uniqueTriangles.reserve(m_AllTriangles.size());
    for (const auto& tri : m_AllTriangles) {
        float area = 0.5f * glm::length(glm::cross(tri.b - tri.a, tri.c - tri.a));
        if (area < 0.001f) {
            continue;
        }
        uniqueTriangles.push_back(tri);
    }
    m_AllTriangles = std::move(uniqueTriangles);
}

bool CollisionWorld::RaycastMap(glm::vec3 origin, glm::vec3 dir, float maxDist, HitResult& out) const {
    bool hitPlane = false;
    float planeT = maxDist;
    glm::vec3 planeNormal(0.0f, 1.0f, 0.0f);
    
    if (m_hasFlatGround && std::abs(dir.y) > 0.0001f) {
        float t = (m_groundY - origin.y) / dir.y;
        if (t >= 0.0f && t <= maxDist) {
            hitPlane = true;
            planeT = t;
            if (dir.y > 0.0f) {
                planeNormal = glm::vec3(0.0f, -1.0f, 0.0f);
            }
        }
    }

    bool bvhHit = m_MapBVH.Raycast(origin, dir, hitPlane ? planeT : maxDist, out);
    if (bvhHit) {
        return true;
    }

    if (hitPlane) {
        out.hit = true;
        out.t = planeT;
        out.point = origin + dir * planeT;
        out.normal = planeNormal;
        return true;
    }

    return false;
}

bool CollisionWorld::SweepAABB(const AABB& box, glm::vec3 delta, HitResult& out) const {
    bool hitPlane = false;
    float planeT = 1.0f;
    glm::vec3 planeNormal(0.0f, 1.0f, 0.0f);

    if (m_hasFlatGround) {
        if (delta.y < -0.0001f) {
            float t = (m_groundY - box.min.y) / delta.y;
            if (t >= 0.0f && t <= 1.0f) {
                hitPlane = true;
                planeT = t;
                planeNormal = glm::vec3(0.0f, 1.0f, 0.0f);
            }
        } else if (delta.y > 0.0001f) {
            float t = (m_groundY - box.max.y) / delta.y;
            if (t >= 0.0f && t <= 1.0f) {
                hitPlane = true;
                planeT = t;
                planeNormal = glm::vec3(0.0f, -1.0f, 0.0f);
            }
        }
    }

    bool bvhHit = m_MapBVH.SweepAABB(box, delta, out);
    if (bvhHit && out.t < (hitPlane ? planeT : 1.0f)) {
        return true;
    }

    if (hitPlane) {
        out.hit = true;
        out.t = planeT;
        out.point = box.GetCenter() + delta * planeT;
        out.normal = planeNormal;
        return true;
    }

    return bvhHit;
}

bool CollisionWorld::OverlapAABB(const AABB& box) const {
    if (m_hasFlatGround && box.min.y <= m_groundY && box.max.y >= m_groundY) {
        return true;
    }
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

bool CollisionWorld::IsPositionOccupied(const glm::vec3& position, float checkRadius) const {
    AABB queryBox;
    queryBox.min = position - glm::vec3(checkRadius);
    queryBox.max = position + glm::vec3(checkRadius);

    for (const auto& tri : m_AllTriangles) {
        float area = 0.5f * glm::length(glm::cross(tri.b - tri.a, tri.c - tri.a));
        if (area > 0.001f) {
            // Check triangle bounding box overlap first
            AABB triBounds = tri.GetBounds();
            if (triBounds.max.x < queryBox.min.x || triBounds.min.x > queryBox.max.x ||
                triBounds.max.y < queryBox.min.y || triBounds.min.y > queryBox.max.y ||
                triBounds.max.z < queryBox.min.z || triBounds.min.z > queryBox.max.z) {
                continue;
            }

            // Approximate distance check to the triangle's vertices or center
            float distA = glm::distance(position, tri.a);
            float distB = glm::distance(position, tri.b);
            float distC = glm::distance(position, tri.c);
            float distCenter = glm::distance(position, tri.GetCenter());
            if (distA < checkRadius || distB < checkRadius || distC < checkRadius || distCenter < checkRadius) {
                return true;
            }

            // Distance to the triangle plane:
            glm::vec3 normal = glm::normalize(glm::cross(tri.b - tri.a, tri.c - tri.a));
            float distToPlane = std::abs(glm::dot(position - tri.a, normal));
            if (distToPlane < checkRadius) {
                // Check if projection of position onto plane lies within triangle
                glm::vec3 proj = position - normal * glm::dot(position - tri.a, normal);
                // Barycentric check:
                glm::vec3 v0 = tri.b - tri.a, v1 = tri.c - tri.a, v2 = proj - tri.a;
                float d00 = glm::dot(v0, v0);
                float d01 = glm::dot(v0, v1);
                float d11 = glm::dot(v1, v1);
                float d20 = glm::dot(v2, v0);
                float d21 = glm::dot(v2, v1);
                float denom = d00 * d11 - d01 * d01;
                if (std::abs(denom) > 0.0001f) {
                    float v = (d11 * d20 - d01 * d21) / denom;
                    float w = (d00 * d21 - d01 * d20) / denom;
                    float u = 1.0f - v - w;
                    if (u >= 0.0f && v >= 0.0f && w >= 0.0f) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}
