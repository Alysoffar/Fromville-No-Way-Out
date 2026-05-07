#pragma once

#include <glm/glm.hpp>

struct AABB {
    glm::vec3 min = glm::vec3(0.0f);
    glm::vec3 max = glm::vec3(0.0f);

    bool Intersects(const AABB& other) const;
    
    glm::vec3 GetCenter() const { return (min + max) * 0.5f; }
    glm::vec3 GetExtents() const { return (max - min) * 0.5f; }
    
    void Expand(const AABB& other) {
        min = glm::min(min, other.min);
        max = glm::max(max, other.max);
    }

    void Expand(const glm::vec3& point) {
        min = glm::min(min, point);
        max = glm::max(max, point);
    }
};

struct Ray {
    glm::vec3 origin;
    glm::vec3 dir;
    glm::vec3 invDir;

    Ray(glm::vec3 o, glm::vec3 d) : origin(o), dir(d) {
        invDir = 1.0f / dir;
    }
};

bool RayAABB(const Ray& ray, const AABB& box, float& tMin, float& tMax);
bool RayTriangle(const Ray& ray, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, float& t, glm::vec3& normal);
bool AABBTriangle(const AABB& box, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2);
bool SweptAABBTriangle(const AABB& box, const glm::vec3& delta, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, float& t, glm::vec3& normal);
