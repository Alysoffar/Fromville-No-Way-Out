#include "game/world/map_manager.h"

#include "engine/physics/HeightmapSampler.h"

void MapManager::Update(const glm::vec3& cameraPosition) {
    (void)cameraPosition;
    origin = glm::vec2(-loopSpan * 0.5f, -loopSpan * 0.5f);
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