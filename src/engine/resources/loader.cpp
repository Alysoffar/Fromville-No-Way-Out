#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Winvalid-constexpr"
#endif

#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobjloader/tiny_obj_loader.h>

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include "engine/resources/loader.h"

#include <fstream>
#include <sstream>

std::string Loader::ReadTextFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        return {};
    }

    std::ostringstream stream;
    stream << file.rdbuf();
    return stream.str();
}

bool Loader::LoadImageRGBA(const std::filesystem::path& path, int& width, int& height, std::vector<unsigned char>& pixels) {
    int channels = 0;
    stbi_uc* data = stbi_load(path.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!data) {
        return false;
    }

    const std::size_t byteCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u;
    pixels.assign(data, data + byteCount);
    stbi_image_free(data);
    return true;
}

bool Loader::LoadOBJ(const std::filesystem::path& path, std::vector<MeshVertex>& vertices, std::vector<unsigned int>& indices) {
    vertices.clear();
    indices.clear();

    tinyobj::ObjReaderConfig config;
    config.mtl_search_path = path.parent_path().string();
    config.triangulate = true;

    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile(path.string(), config)) {
        return false;
    }

    const tinyobj::attrib_t& attrib = reader.GetAttrib();
    const std::vector<tinyobj::shape_t>& shapes = reader.GetShapes();

    if (attrib.vertices.empty() || shapes.empty()) {
        return false;
    }

    vertices.reserve(4096);
    indices.reserve(12288);

    // tinyobj gives us arbitrary indexed faces; we'll emit one vertex per index
    for (const tinyobj::shape_t& shape : shapes) {
        const auto& idx = shape.mesh.indices;
        // process triangles (tinyobj triangulate config true)
        for (size_t i = 0; i + 2 < idx.size(); i += 3) {
            // get positions
            glm::vec3 p[3];
            for (int k = 0; k < 3; ++k) {
                const tinyobj::index_t& ii = idx[i + k];
                const std::size_t vbase = static_cast<std::size_t>(ii.vertex_index) * 3u;
                p[k] = glm::vec3(
                    attrib.vertices[vbase],
                    attrib.vertices[vbase + 1u],
                    attrib.vertices[vbase + 2u]
                );
            }

            // compute face normal
            glm::vec3 fn = glm::normalize(glm::cross(p[1] - p[0], p[2] - p[0]));

            for (int k = 0; k < 3; ++k) {
                const tinyobj::index_t& ii = idx[i + k];
                MeshVertex vertex;
                const std::size_t vbase = static_cast<std::size_t>(ii.vertex_index) * 3u;
                vertex.position = glm::vec3(
                    attrib.vertices[vbase],
                    attrib.vertices[vbase + 1u],
                    attrib.vertices[vbase + 2u]
                );

                if (!attrib.normals.empty() && ii.normal_index >= 0) {
                    const std::size_t nbase = static_cast<std::size_t>(ii.normal_index) * 3u;
                    vertex.normal = glm::vec3(
                        attrib.normals[nbase],
                        attrib.normals[nbase + 1u],
                        attrib.normals[nbase + 2u]
                    );
                } else {
                    vertex.normal = fn;
                }

                // default color; can be replaced by material parsing later
                vertex.color = glm::vec3(0.8f);

                vertices.push_back(vertex);
                indices.push_back(static_cast<unsigned int>(vertices.size() - 1u));
            }
        }
    }

    return !vertices.empty() && !indices.empty();
}