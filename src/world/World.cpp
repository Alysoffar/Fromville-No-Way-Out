#include "World.h"

#include "world/DayNightCycle.h"
#include "world/WorldBuilder.h"

World::World()
    : builder(std::make_unique<WorldBuilder>(nullptr, nullptr)) {}

World::~World() = default;

void World::Load(const std::string& path) {
    (void)path;
    if (!builder) {
        builder = std::make_unique<WorldBuilder>(nullptr, nullptr);
    }
    builder->BuildAll();
}

void World::Draw(Renderer& renderer, Camera& camera) {
    if (!builder) {
        return;
    }
    builder->DrawAll(renderer, camera, DayNightCycle::Get());
}
