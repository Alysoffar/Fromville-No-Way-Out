#pragma once

#include <glm/glm.hpp>

class MapManager {
public:
    void Update(const glm::vec3& cameraPosition);

    glm::vec2 GetOrigin() const;
    int GetResolution() const;
    float GetCellSize() const;
    float SampleHeight(float worldX, float worldZ) const;

private:
    glm::vec2 origin = glm::vec2(0.0f);
    int resolution = 64;
    float cellSize = 1.5f;
    float loopSpan = 96.0f;
};