#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "engine/renderer/Mesh.h"

struct OBJShape {
    std::string name;
    std::vector<MeshVertex> vertices;
    std::vector<unsigned int> indices;
};

class Loader {
public:
    static std::string ReadTextFile(const std::filesystem::path& path);
    static bool LoadImageRGBA(const std::filesystem::path& path, int& width, int& height, std::vector<unsigned char>& pixels);
    static bool LoadOBJ(const std::filesystem::path& path, std::vector<MeshVertex>& vertices, std::vector<unsigned int>& indices);
    static bool LoadOBJWithShapes(const std::filesystem::path& path, std::vector<OBJShape>& shapes);
};