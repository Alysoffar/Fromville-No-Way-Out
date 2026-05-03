#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "engine/resources/loader.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

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

    std::cout << "[Loader] Loading model with Assimp: " << path.string() << "\n";

    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(
        path.string(),
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_JoinIdenticalVertices |
        aiProcess_OptimizeMeshes
    );

    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
        std::cerr << "[Loader] Assimp error: " << importer.GetErrorString() << "\n";
        return false;
    }

    std::cout << "[Loader] Scene loaded: " << scene->mNumMeshes << " mesh(es)\n";

    // Count total vertices/indices for pre-allocation
    size_t totalVertices = 0;
    size_t totalIndices = 0;
    for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
        const aiMesh* mesh = scene->mMeshes[m];
        totalVertices += mesh->mNumVertices;
        for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
            totalIndices += mesh->mFaces[f].mNumIndices;
        }
    }

    vertices.reserve(totalVertices);
    indices.reserve(totalIndices);

    glm::vec3 minPos(1e10f);
    glm::vec3 maxPos(-1e10f);

    // Process all meshes in the scene
    unsigned int indexOffset = 0;
    for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
        const aiMesh* mesh = scene->mMeshes[m];

        // Extract vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
            MeshVertex vertex;

            vertex.position = glm::vec3(
                mesh->mVertices[i].x,
                mesh->mVertices[i].y,
                mesh->mVertices[i].z
            );

            if (mesh->HasNormals()) {
                vertex.normal = glm::vec3(
                    mesh->mNormals[i].x,
                    mesh->mNormals[i].y,
                    mesh->mNormals[i].z
                );
            } else {
                vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
            }

            // Use vertex colors if available, otherwise default gray
            if (mesh->HasVertexColors(0)) {
                vertex.color = glm::vec3(
                    mesh->mColors[0][i].r,
                    mesh->mColors[0][i].g,
                    mesh->mColors[0][i].b
                );
            } else {
                vertex.color = glm::vec3(0.8f);
            }

            minPos = glm::min(minPos, vertex.position);
            maxPos = glm::max(maxPos, vertex.position);

            vertices.push_back(vertex);
        }

        // Extract indices
        for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
            const aiFace& face = mesh->mFaces[f];
            for (unsigned int j = 0; j < face.mNumIndices; ++j) {
                indices.push_back(indexOffset + face.mIndices[j]);
            }
        }

        indexOffset += mesh->mNumVertices;
    }

    bool success = !vertices.empty() && !indices.empty();
    if (success) {
        std::cout << "[Loader] Loaded: " << path.string() << "\n";
        std::cout << "  Vertices: " << vertices.size()
                  << ", Indices: " << indices.size() << "\n";
        std::cout << "  Meshes: " << scene->mNumMeshes << "\n";
        std::cout << "  Bounding box: min(" << minPos.x << ", " << minPos.y << ", " << minPos.z << ")"
                  << " max(" << maxPos.x << ", " << maxPos.y << ", " << maxPos.z << ")\n";
    }
    return success;
}
