#include "engine/physics/Collision.h"
#include <algorithm>
#include <glm/gtc/epsilon.hpp>

bool AABB::Intersects(const AABB& other) const {
    return min.x <= other.max.x && max.x >= other.min.x &&
           min.y <= other.max.y && max.y >= other.min.y &&
           min.z <= other.max.z && max.z >= other.min.z;
}

bool RayAABB(const Ray& ray, const AABB& box, float& tMin, float& tMax) {
    float t1 = (box.min.x - ray.origin.x) * ray.invDir.x;
    float t2 = (box.max.x - ray.origin.x) * ray.invDir.x;
    float t3 = (box.min.y - ray.origin.y) * ray.invDir.y;
    float t4 = (box.max.y - ray.origin.y) * ray.invDir.y;
    float t5 = (box.min.z - ray.origin.z) * ray.invDir.z;
    float t6 = (box.max.z - ray.origin.z) * ray.invDir.z;

    tMin = std::max(std::max(std::min(t1, t2), std::min(t3, t4)), std::min(t5, t6));
    tMax = std::min(std::min(std::max(t1, t2), std::max(t3, t4)), std::max(t5, t6));

    return tMax >= 0 && tMin <= tMax;
}

bool RayTriangle(const Ray& ray, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, float& t, glm::vec3& normal) {
    const float EPSILON = 0.0000001f;
    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;
    glm::vec3 h = glm::cross(ray.dir, edge2);
    float a = glm::dot(edge1, h);

    if (a > -EPSILON && a < EPSILON) return false;

    float f = 1.0f / a;
    glm::vec3 s = ray.origin - v0;
    float u = f * glm::dot(s, h);

    if (u < 0.0f || u > 1.0f) return false;

    glm::vec3 q = glm::cross(s, edge1);
    float v = f * glm::dot(ray.dir, q);

    if (v < 0.0f || u + v > 1.0f) return false;

    t = f * glm::dot(edge2, q);
    if (t > EPSILON) {
        normal = glm::normalize(glm::cross(edge1, edge2));
        if (glm::dot(normal, ray.dir) > 0) normal = -normal;
        return true;
    }
    return false;
}

bool AABBTriangle(const AABB& box, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2) {
    glm::vec3 center = box.GetCenter();
    glm::vec3 extents = box.GetExtents();

    glm::vec3 tv0 = v0 - center;
    glm::vec3 tv1 = v1 - center;
    glm::vec3 tv2 = v2 - center;

    glm::vec3 e0 = tv1 - tv0;
    glm::vec3 e1 = tv2 - tv1;
    glm::vec3 e2 = tv0 - tv2;

    glm::vec3 axes[13] = {
        {1, 0, 0}, {0, 1, 0}, {0, 0, 1},
        glm::normalize(glm::cross(e0, e1)),
        glm::cross({1, 0, 0}, e0), glm::cross({1, 0, 0}, e1), glm::cross({1, 0, 0}, e2),
        glm::cross({0, 1, 0}, e0), glm::cross({0, 1, 0}, e1), glm::cross({0, 1, 0}, e2),
        glm::cross({0, 0, 1}, e0), glm::cross({0, 0, 1}, e1), glm::cross({0, 0, 1}, e2)
    };

    for (const auto& axis : axes) {
        float l = glm::length(axis);
        if (l < 0.0001f) continue;
        glm::vec3 nAxis = axis / l;
        
        float r = extents.x * std::abs(nAxis.x) + extents.y * std::abs(nAxis.y) + extents.z * std::abs(nAxis.z);
        float p0 = glm::dot(tv0, nAxis);
        float p1 = glm::dot(tv1, nAxis);
        float p2 = glm::dot(tv2, nAxis);

        float tMin = std::min({p0, p1, p2});
        float tMax = std::max({p0, p1, p2});

        if (tMin > r || tMax < -r) return false;
    }

    return true;
}

bool SweptAABBTriangle(const AABB& box, const glm::vec3& delta, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, float& t, glm::vec3& normal) {
    float t_entry = -1e30f;
    float t_exit = 1e30f;
    
    glm::vec3 center = box.GetCenter();
    glm::vec3 extents = box.GetExtents();
    
    glm::vec3 tv0 = v0 - center;
    glm::vec3 tv1 = v1 - center;
    glm::vec3 tv2 = v2 - center;
    
    glm::vec3 e0 = tv1 - tv0;
    glm::vec3 e1 = tv2 - tv1;
    glm::vec3 e2 = tv0 - tv2;
    
    glm::vec3 triNormal = glm::cross(e0, e1);
    
    glm::vec3 axes[13] = {
        {1, 0, 0}, {0, 1, 0}, {0, 0, 1},
        triNormal,
        glm::cross({1, 0, 0}, e0), glm::cross({1, 0, 0}, e1), glm::cross({1, 0, 0}, e2),
        glm::cross({0, 1, 0}, e0), glm::cross({0, 1, 0}, e1), glm::cross({0, 1, 0}, e2),
        glm::cross({0, 0, 1}, e0), glm::cross({0, 0, 1}, e1), glm::cross({0, 0, 1}, e2)
    };
    
    glm::vec3 hitNormal = glm::normalize(triNormal);
    
    for (const auto& axis : axes) {
        float l = glm::length(axis);
        if (l < 0.0001f) continue;
        glm::vec3 nAxis = axis / l;
        
        float r = extents.x * std::abs(nAxis.x) + extents.y * std::abs(nAxis.y) + extents.z * std::abs(nAxis.z);
        float p0 = glm::dot(tv0, nAxis);
        float p1 = glm::dot(tv1, nAxis);
        float p2 = glm::dot(tv2, nAxis);
        
        float triMin = std::min({p0, p1, p2});
        float triMax = std::max({p0, p1, p2});
        
        float vProj = glm::dot(delta, nAxis);
        
        if (std::abs(vProj) < 0.0001f) {
            if (triMin > r || triMax < -r) return false;
        } else {
            float t0 = (triMin - r) / vProj;
            float t1 = (triMax + r) / vProj;
            
            if (t0 > t1) std::swap(t0, t1);
            
            if (t0 > t_entry) {
                t_entry = t0;
                hitNormal = (vProj > 0) ? -nAxis : nAxis;
            }
            if (t1 < t_exit) t_exit = t1;
            
            if (t_entry > t_exit) return false;
        }
    }
    
    if (t_entry >= 0.0f && t_entry < 1.0f) {
        t = t_entry;
        normal = hitNormal;
        return true;
    }
    
    return false;
}