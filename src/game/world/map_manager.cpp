#include "game/world/map_manager.h"

#include "engine/physics/HeightmapSampler.h"

void MapManager::Update(const glm::vec3& cameraPosition) {
    const float snappedX = std::floor(cameraPosition.x / loopSpan) * loopSpan;
    const float snappedZ = std::floor(cameraPosition.z / loopSpan) * loopSpan;
    origin = glm::vec2(snappedX, snappedZ);
}

glm::vec2 MapManager::GetOrigin() const {
    return origin;
}

int MapManager::GetResolution() const {
    return resolution;
}

float MapManager::GetCellSize() const {
    return cellSize;
}

float MapManager::SampleHeight(float worldX, float worldZ) const {
    return HeightmapSampler::Sample(worldX, worldZ);
}