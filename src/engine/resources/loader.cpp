#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "engine/resources/loader.h"

#include <algorithm>
#include <fstream>
#include <iostream>
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

    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[Loader] Failed to open OBJ: " << path << "\n";
        return false;
    }

    // -------------------------------------------------------------------------
    // Raw data storage while scanning the file
    // -------------------------------------------------------------------------
    std::vector<glm::vec3> positions;   // from "v" lines
    std::vector<glm::vec3> normals;     // from "vn" lines
    std::string line;
    int lineNumber = 0;

    struct FaceVert {
        int v  = -1;   // position index  (0-based after Fix 1)
        int vn = -1;   // normal index    (0-based after Fix 1)
    };
    std::vector<std::vector<FaceVert>> faces;  // each face = list of corner indices

    // -------------------------------------------------------------------------
    // Pass 1: read every line, store positions/normals/faces
    // -------------------------------------------------------------------------
    while (std::getline(file, line)) {
        ++lineNumber;
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "v") {
            float x, y, z;
            if (iss >> x >> y >> z) {
                positions.emplace_back(x, y, z);
            }
        } else if (prefix == "vn") {
            float x, y, z;
            if (iss >> x >> y >> z) {
                normals.emplace_back(x, y, z);
            }
        } else if (prefix == "f") {
            std::vector<FaceVert> face;
            std::string vertToken;
            while (iss >> vertToken) {
                FaceVert fv;
                // Face vertex formats: v  |  v/vt  |  v/vt/vn  |  v//vn
                // Replace '/' with ' ' so stringstream can parse the fields
                std::replace(vertToken.begin(), vertToken.end(), '/', ' ');
                std::istringstream viss(vertToken);

                // Read position index
                if (viss >> fv.v) {
                    // Fix 1: OBJ indices are 1-based; subtract 1 for C++ arrays
                    fv.v -= 1;
                }
                // Skip optional texcoord
                int vtDummy;
                if (viss >> vtDummy) {
                    // texcoord present but unused
                }
                // Read optional normal index
                if (viss >> fv.vn) {
                    // Fix 1: normal indices are also 1-based
                    fv.vn -= 1;
                }
                face.push_back(fv);
            }
            faces.push_back(std::move(face));
        }
    }

    // -------------------------------------------------------------------------
    // Pass 2: triangulate, bounds-check, and build interleaved vertex buffer
    // -------------------------------------------------------------------------
    glm::vec3 minPos(1e10f);
    glm::vec3 maxPos(-1e10f);

    for (size_t faceIdx = 0; faceIdx < faces.size(); ++faceIdx) {
        const auto& face = faces[faceIdx];

        // Fix 2: Detect quads / n-gons
        if (face.size() < 3) {
            std::cerr << "[Loader] Warning: face " << faceIdx
                      << " has fewer than 3 vertices, skipping.\n";
            continue;
        }
        if (face.size() > 3) {
            std::cerr << "[Loader] Warning: face " << faceIdx
                      << " has " << face.size()
                      << " vertices (quad/ngon). Fan-triangulating.\n";
        }

        // Fix 2: Fan triangulation
        //   quad  f a b c d  ->  triangles (a,b,c) and (a,c,d)
        //   n-gon f a b c d e -> (a,b,c), (a,c,d), (a,d,e), ...
        for (size_t i = 1; i + 1 < face.size(); ++i) {
            const FaceVert tri[3] = {face[0], face[i], face[i + 1]};

            // Compute fallback face-normal now (before any index may be rejected)
            glm::vec3 p[3];
            for (int k = 0; k < 3; ++k) {
                if (tri[k].v >= 0 && tri[k].v < static_cast<int>(positions.size())) {
                    p[k] = positions[tri[k].v];
                } else {
                    p[k] = glm::vec3(0.0f);
                }
            }
            glm::vec3 faceNormal = glm::normalize(glm::cross(p[1] - p[0], p[2] - p[0]));

            for (int k = 0; k < 3; ++k) {
                // Fix 3: Bounds check — every index must be < vertex count
                if (tri[k].v < 0 || tri[k].v >= static_cast<int>(positions.size())) {
                    std::cerr << "[Loader] ERROR: face " << faceIdx
                              << " vertex " << k
                              << " has out-of-bounds index " << tri[k].v
                              << " (valid: 0.." << static_cast<int>(positions.size()) - 1
                              << ", line ~" << lineNumber << ")\n";
                    vertices.clear();
                    indices.clear();
                    return false;
                }

                MeshVertex vertex;
                vertex.position = positions[tri[k].v];

                if (tri[k].vn >= 0 && tri[k].vn < static_cast<int>(normals.size())) {
                    vertex.normal = normals[tri[k].vn];
                } else {
                    vertex.normal = faceNormal;
                }

                vertex.color = glm::vec3(0.8f);

                minPos = glm::min(minPos, vertex.position);
                maxPos = glm::max(maxPos, vertex.position);

                vertices.push_back(vertex);
                indices.push_back(static_cast<unsigned int>(indices.size()));
            }
        }
    }

    bool success = !vertices.empty() && !indices.empty();
    if (success) {
        std::cout << "[Loader] Loaded OBJ: " << path.string() << "\n";
        std::cout << "  Vertices: " << vertices.size()
                  << ", Indices: " << indices.size() << "\n";
        std::cout << "  Faces parsed: " << faces.size() << "\n";
        std::cout << "  Bounding box: min(" << minPos.x << ", " << minPos.y << ", " << minPos.z << ")"
                  << " max(" << maxPos.x << ", " << maxPos.y << ", " << maxPos.z << ")\n";
    }
    return success;
}
