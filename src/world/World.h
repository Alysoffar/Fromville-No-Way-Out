#pragma once
#include <string>

class Renderer;
class Camera;

class World {
public:
    void Load(const std::string& path) {}
    void Draw(Renderer& renderer, Camera& camera) {}
};
