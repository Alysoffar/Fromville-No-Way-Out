#pragma once

#include <string>

#include <memory>

class Renderer;
class Camera;
class WorldBuilder;

class World {
public:
    World();
    ~World();

    void Load(const std::string& path);
    void Draw(Renderer& renderer, Camera& camera);

private:
    std::unique_ptr<WorldBuilder> builder;
};
