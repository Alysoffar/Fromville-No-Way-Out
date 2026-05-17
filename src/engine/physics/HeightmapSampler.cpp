#include "engine/physics/HeightmapSampler.h"

#include <cmath>

float HeightmapSampler::Sample(float worldX, float worldZ) {
    const float valleyWave = std::sin(worldX * 0.06f) * 1.3f;
    const float ridgeWave = std::cos(worldZ * 0.045f) * 0.9f;
    const float basinWave = std::sin((worldX + worldZ) * 0.03f) * 0.5f;
    return valleyWave + ridgeWave - basinWave - 1.5f;
}