#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobjloader/tiny_obj_loader.h>

#include "engine/resources/loader.h"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>

// =============================================================================
// ReadTextFile / LoadImageRGBA — unchanged from previous version
// =============================================================================

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

// =============================================================================
// Binary Cache
// =============================================================================
//
// After the first successful tinyobjloader parse, the processed vertex + index
// buffers are serialised to a .bin file next to the .obj.  On subsequent loads
// we check whether the cache file exists and is newer than the .obj — if so,
// we read it directly, bypassing tinyobjloader entirely.
//
// Cache format (little-endian, tightly packed):
//
//   Offset  Size       Field
//   ------  ---------  ---------------------------
//   0       4 bytes    Magic number  (0x464D5348 = "FMSH")
//   4       4 bytes    Version       (currently 1)
//   8       8 bytes    Vertex count  (uint64_t)
//   16      8 bytes    Index count   (uint64_t)
//   24      V bytes    Raw vertex data  (vertexCount * sizeof(MeshVertex))
//   24+V    I bytes    Raw index data   (indexCount  * sizeof(uint32_t))
//
// =============================================================================

namespace {

constexpr uint32_t kCacheMagic   = 0x464D5348u; // "FMSH"
constexpr uint32_t kCacheVersion = 1u;

// Build the cache path by replacing .obj extension with .bin
std::filesystem::path CachePathFor(const std::filesystem::path& objPath) {
    auto p = objPath;
    p.replace_extension(".bin");
    return p;
}

// Returns true if the cache file exists and is at least as new as the OBJ.
bool IsCacheValid(const std::filesystem::path& objPath, const std::filesystem::path& cachePath) {
    std::error_code ec;
    if (!std::filesystem::exists(cachePath, ec)) return false;
    if (!std::filesystem::exists(objPath, ec))   return false;

    auto objTime   = std::filesystem::last_write_time(objPath, ec);
    if (ec) return false;
    auto cacheTime = std::filesystem::last_write_time(cachePath, ec);
    if (ec) return false;

    return cacheTime >= objTime;
}

// Try to load from the binary cache. Returns true on success.
bool LoadFromCache(const std::filesystem::path& cachePath,
                   std::vector<MeshVertex>& vertices,
                   std::vector<unsigned int>& indices) {
    std::ifstream file(cachePath, std::ios::binary);
    if (!file.is_open()) return false;

    // Read and validate header
    uint32_t magic = 0, version = 0;
    uint64_t vertexCount = 0, indexCount = 0;

    file.read(reinterpret_cast<char*>(&magic),       sizeof(magic));
    file.read(reinterpret_cast<char*>(&version),     sizeof(version));
    file.read(reinterpret_cast<char*>(&vertexCount), sizeof(vertexCount));
    file.read(reinterpret_cast<char*>(&indexCount),  sizeof(indexCount));

    if (!file || magic != kCacheMagic || version != kCacheVersion) {
        std::cerr << "[Loader] Cache header mismatch, will re-parse OBJ.\n";
        return false;
    }

    // Sanity check — reject obviously corrupt sizes (> 1 billion elements)
    if (vertexCount > 1'000'000'000ull || indexCount > 1'000'000'000ull) {
        std::cerr << "[Loader] Cache has unreasonable counts, will re-parse OBJ.\n";
        return false;
    }

    vertices.resize(static_cast<size_t>(vertexCount));
    indices.resize(static_cast<size_t>(indexCount));

    file.read(reinterpret_cast<char*>(vertices.data()),
              static_cast<std::streamsize>(vertexCount * sizeof(MeshVertex)));
    file.read(reinterpret_cast<char*>(indices.data()),
              static_cast<std::streamsize>(indexCount * sizeof(unsigned int)));

    if (!file) {
        std::cerr << "[Loader] Cache read incomplete, will re-parse OBJ.\n";
        vertices.clear();
        indices.clear();
        return false;
    }

    return true;
}

// Write the binary cache after a successful parse.
bool WriteCache(const std::filesystem::path& cachePath,
                const std::vector<MeshVertex>& vertices,
                const std::vector<unsigned int>& indices) {
    std::ofstream file(cachePath, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        std::cerr << "[Loader] Warning: could not write cache to " << cachePath << "\n";
        return false;
    }

    const uint32_t magic   = kCacheMagic;
    const uint32_t version = kCacheVersion;
    const uint64_t vertexCount = vertices.size();
    const uint64_t indexCount  = indices.size();

    file.write(reinterpret_cast<const char*>(&magic),       sizeof(magic));
    file.write(reinterpret_cast<const char*>(&version),     sizeof(version));
    file.write(reinterpret_cast<const char*>(&vertexCount), sizeof(vertexCount));
    file.write(reinterpret_cast<const char*>(&indexCount),  sizeof(indexCount));

    file.write(reinterpret_cast<const char*>(vertices.data()),
               static_cast<std::streamsize>(vertexCount * sizeof(MeshVertex)));
    file.write(reinterpret_cast<const char*>(indices.data()),
               static_cast<std::streamsize>(indexCount * sizeof(unsigned int)));

    return file.good();
}

// -------------------------------------------------------------------------
// Vertex deduplication key
// -------------------------------------------------------------------------
// MeshVertex has position + normal + color (no UV), so the deduplication key
// is (vertex_index, normal_index).  texcoord_index is intentionally excluded
// because there is no UV field in MeshVertex — including it would cause
// phantom deduplication splits where identical (v, vn) combos get separate
// vertices just because the OBJ listed different vt indices.   (Fix #5)
// -------------------------------------------------------------------------
struct VertexKey {
    int v;  // position index into attrib.vertices
    int n;  // normal index into attrib.normals
};

struct VertexKeyEqual {
    bool operator()(const VertexKey& a, const VertexKey& b) const {
        return a.v == b.v && a.n == b.n;
    }
};

// Fix #2 — FNV-1a hash to minimise collisions on large meshes.
// The old XOR-with-small-shifts hash produced heavy collision rates because
// adjacent index values differ by 1 and the shifts were too small.
struct VertexKeyHash {
    size_t operator()(const VertexKey& k) const {
        size_t h = 2166136261u;
        h = (h ^ static_cast<size_t>(k.v)) * 16777619u;
        h = (h ^ static_cast<size_t>(k.n)) * 16777619u;
        return h;
    }
};

} // anonymous namespace

// =============================================================================
// LoadOBJ — tinyobjloader parse with single-pass dedup + binary cache
// =============================================================================

bool Loader::LoadOBJ(const std::filesystem::path& path,
                     std::vector<MeshVertex>& vertices,
                     std::vector<unsigned int>& indices) {
    vertices.clear();
    indices.clear();

    // -----------------------------------------------------------------
    // Step 1: Try the binary cache first.
    // If a .bin file exists alongside the .obj and is newer, load from
    // it directly — this skips tinyobjloader parsing entirely.
    // -----------------------------------------------------------------
    const auto cachePath = CachePathFor(path);
    if (IsCacheValid(path, cachePath)) {
        if (LoadFromCache(cachePath, vertices, indices)) {
            std::cout << "[Loader] Loaded from binary cache: " << cachePath.string() << "\n";
            std::cout << "  Vertices: " << vertices.size()
                      << ", Indices: " << indices.size() << "\n";
            return true;
        }
        // Cache was corrupt / version mismatch — fall through to re-parse.
    }

    // -----------------------------------------------------------------
    // Step 2: Parse the OBJ with tinyobjloader.
    // -----------------------------------------------------------------
    std::cout << "[Loader] Parsing OBJ with tinyobjloader: " << path.string() << "\n";

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    // Use the directory containing the .obj as the material search path
    std::string mtlBaseDir = path.parent_path().string();
    if (!mtlBaseDir.empty() && mtlBaseDir.back() != '/' && mtlBaseDir.back() != '\\') {
        mtlBaseDir += '/';
    }

    bool ok = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                               path.string().c_str(), mtlBaseDir.c_str());

    if (!warn.empty()) {
        std::cerr << "[Loader] tinyobj warning: " << warn << "\n";
    }
    if (!err.empty()) {
        std::cerr << "[Loader] tinyobj error: " << err << "\n";
    }
    if (!ok) {
        std::cerr << "[Loader] Failed to load OBJ: " << path << "\n";
        return false;
    }

    // -----------------------------------------------------------------
    // Step 3: Estimate total index count so we can reserve up-front.
    // -----------------------------------------------------------------
    size_t totalIndices = 0;
    for (const auto& shape : shapes) {
        totalIndices += shape.mesh.indices.size();
    }
    // A reasonable upper-bound guess: at most every index is unique.
    vertices.reserve(totalIndices);
    indices.reserve(totalIndices);

    // -----------------------------------------------------------------
    // Step 4: Single-pass vertex deduplication.
    //
    // Fix #1: The old code iterated mesh.indices TWICE per shape (once
    // to build unique vertices, once to build the index buffer) and did
    // TWO hash-map lookups per iteration.  We collapse this into a
    // single pass using try_emplace — each index is touched exactly once,
    // and try_emplace does at most one lookup+insert.
    //
    // Fix #3: shapeVertices and uniqueVertices are reserved at the start
    // of each shape iteration to avoid repeated reallocations.
    // -----------------------------------------------------------------

    // -----------------------------------------------------------------
    // Build a material color lookup from tinyobjloader materials.
    // If the MTL Kd is the Blender default grey (0.8, 0.8, 0.8),
    // assign a semantic color based on material name keywords instead.
    // -----------------------------------------------------------------
    auto getSemanticColor = [](const std::string& name) -> glm::vec3 {
        // Convert to lowercase for matching
        std::string lower = name;
        for (auto& c : lower) c = static_cast<char>(std::tolower(c));

        // Trees
        if (lower.find("bark") != std::string::npos)      return glm::vec3(0.40f, 0.26f, 0.13f);
        if (lower.find("trunk") != std::string::npos)     return glm::vec3(0.40f, 0.26f, 0.13f);
        if (lower.find("leaf") != std::string::npos)      return glm::vec3(0.15f, 0.45f, 0.10f);
        // Buildings
        if (lower.find("roof") != std::string::npos)      return glm::vec3(0.55f, 0.22f, 0.12f);
        if (lower.find("wood") != std::string::npos)      return glm::vec3(0.55f, 0.40f, 0.25f);
        if (lower.find("door") != std::string::npos)      return glm::vec3(0.45f, 0.30f, 0.18f);
        if (lower.find("window") != std::string::npos)    return glm::vec3(0.60f, 0.75f, 0.85f);
        if (lower.find("concrete") != std::string::npos)  return glm::vec3(0.65f, 0.63f, 0.60f);
        if (lower.find("slab") != std::string::npos)      return glm::vec3(0.60f, 0.58f, 0.55f);
        // Road
        if (lower.find("material.002") != std::string::npos) return glm::vec3(0.30f, 0.30f, 0.28f);
        if (lower.find("material.001") != std::string::npos) return glm::vec3(0.50f, 0.45f, 0.38f);
        if (lower.find("road") != std::string::npos)      return glm::vec3(0.30f, 0.30f, 0.28f);
        if (lower.find("asphalt") != std::string::npos)   return glm::vec3(0.25f, 0.25f, 0.25f);
        // Grass
        if (lower.find("grass") != std::string::npos)     return glm::vec3(0.20f, 0.50f, 0.12f);
        // Light stand / metal
        if (lower.find("metal") != std::string::npos)     return glm::vec3(0.35f, 0.35f, 0.38f);
        if (lower.find("light") != std::string::npos)     return glm::vec3(0.35f, 0.35f, 0.38f);
        if (lower.find("lamp") != std::string::npos)      return glm::vec3(0.90f, 0.85f, 0.60f);
        // Default
        return glm::vec3(0.7f, 0.7f, 0.7f);
    };

    // Build per-material color array
    std::vector<glm::vec3> matColors;
    matColors.reserve(materials.size());
    for (const auto& mat : materials) {
        glm::vec3 kd(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]);
        // Check if Kd is default Blender grey (0.8, 0.8, 0.8)
        bool isDefaultGrey = (std::abs(kd.r - 0.8f) < 0.01f &&
                              std::abs(kd.g - 0.8f) < 0.01f &&
                              std::abs(kd.b - 0.8f) < 0.01f);
        if (isDefaultGrey) {
            matColors.push_back(getSemanticColor(mat.name));
        } else {
            matColors.push_back(kd);
        }
    }

    glm::vec3 minPos(1e10f);
    glm::vec3 maxPos(-1e10f);

    for (const auto& shape : shapes) {
        const auto& mesh = shape.mesh;
        const size_t numIndices = mesh.indices.size();

        // Fix #3 — pre-reserve per-shape temporaries
        std::vector<MeshVertex> shapeVertices;
        shapeVertices.reserve(numIndices / 2);

        std::unordered_map<VertexKey, unsigned int, VertexKeyHash, VertexKeyEqual> uniqueVertices;
        uniqueVertices.reserve(numIndices / 2);

        for (size_t i = 0; i < numIndices; ++i) {
            const tinyobj::index_t& idx = mesh.indices[i];

            // Determine per-face material color
            size_t faceIndex = i / 3;
            glm::vec3 faceColor(0.7f);
            if (faceIndex < mesh.material_ids.size()) {
                int matId = mesh.material_ids[faceIndex];
                if (matId >= 0 && static_cast<size_t>(matId) < matColors.size()) {
                    faceColor = matColors[matId];
                }
            }

            // Build the deduplication key (position + normal only; no UV — Fix #5)
            VertexKey key;
            key.v = idx.vertex_index;
            key.n = idx.normal_index;

            // Fix #1 — single lookup via try_emplace.
            auto [it, inserted] = uniqueVertices.try_emplace(
                key, static_cast<unsigned int>(shapeVertices.size()));

            if (inserted) {
                MeshVertex vertex;

                // Position (3 floats per vertex)
                if (key.v >= 0 && static_cast<size_t>(key.v * 3 + 2) < attrib.vertices.size()) {
                    vertex.position = glm::vec3(
                        attrib.vertices[3 * key.v + 0],
                        attrib.vertices[3 * key.v + 1],
                        attrib.vertices[3 * key.v + 2]
                    );
                }

                // Normal (3 floats per normal)
                if (key.n >= 0 && static_cast<size_t>(key.n * 3 + 2) < attrib.normals.size()) {
                    vertex.normal = glm::vec3(
                        attrib.normals[3 * key.n + 0],
                        attrib.normals[3 * key.n + 1],
                        attrib.normals[3 * key.n + 2]
                    );
                } else {
                    vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
                }

                // Texture coordinates (2 floats per texcoord)
                if (idx.texcoord_index >= 0 && static_cast<size_t>(idx.texcoord_index * 2 + 1) < attrib.texcoords.size()) {
                    vertex.uv = glm::vec2(
                        attrib.texcoords[2 * idx.texcoord_index + 0],
                        attrib.texcoords[2 * idx.texcoord_index + 1]
                    );
                } else {
                    vertex.uv = glm::vec2(0.0f, 0.0f);
                }

                // Bake material color into vertex color
                vertex.color = faceColor;

                minPos = glm::min(minPos, vertex.position);
                maxPos = glm::max(maxPos, vertex.position);

                shapeVertices.push_back(vertex);
            }

            // Emit the (potentially reused) index, offset by the current
            // global vertex count so shapes don't overlap.
            indices.push_back(static_cast<unsigned int>(vertices.size()) + it->second);
        }

        // Append this shape's unique vertices into the global buffer.
        vertices.insert(vertices.end(), shapeVertices.begin(), shapeVertices.end());
    }

    bool success = !vertices.empty() && !indices.empty();
    if (success) {
        std::cout << "[Loader] Loaded OBJ: " << path.string() << "\n";
        std::cout << "  Vertices: " << vertices.size()
                  << ", Indices: " << indices.size() << "\n";
        std::cout << "  Shapes: " << shapes.size() << "\n";
        std::cout << "  Bounding box: min(" << minPos.x << ", " << minPos.y << ", " << minPos.z << ")"
                  << " max(" << maxPos.x << ", " << maxPos.y << ", " << maxPos.z << ")\n";

        // -----------------------------------------------------------------
        // Step 5: Write binary cache for future loads.
        // -----------------------------------------------------------------
        if (WriteCache(cachePath, vertices, indices)) {
            std::cout << "[Loader] Binary cache written: " << cachePath.string() << "\n";
        }
    }
    return success;
}

bool Loader::LoadOBJWithShapes(const std::filesystem::path& path, std::vector<OBJShape>& shapes) {
    shapes.clear();

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> t_shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    std::string mtlBaseDir = path.parent_path().string();
    if (!mtlBaseDir.empty() && mtlBaseDir.back() != '/' && mtlBaseDir.back() != '\\') {
        mtlBaseDir += '/';
    }

    bool ok = tinyobj::LoadObj(&attrib, &t_shapes, &materials, &warn, &err,
                               path.string().c_str(), mtlBaseDir.c_str());

    if (!ok) {
        std::cerr << "[Loader] Failed to load OBJ: " << path << "\n";
        return false;
    }

    // Material color helper (reuse from LoadOBJ if possible, but for now just copy)
    auto getSemanticColor = [](const std::string& name) -> glm::vec3 {
        std::string lower = name;
        for (auto& c : lower) c = static_cast<char>(std::tolower(c));
        if (lower.find("bark") != std::string::npos)      return glm::vec3(0.40f, 0.26f, 0.13f);
        if (lower.find("trunk") != std::string::npos)     return glm::vec3(0.40f, 0.26f, 0.13f);
        if (lower.find("leaf") != std::string::npos)      return glm::vec3(0.15f, 0.45f, 0.10f);
        if (lower.find("roof") != std::string::npos)      return glm::vec3(0.55f, 0.22f, 0.12f);
        if (lower.find("wood") != std::string::npos)      return glm::vec3(0.55f, 0.40f, 0.25f);
        if (lower.find("door") != std::string::npos)      return glm::vec3(0.45f, 0.30f, 0.18f);
        if (lower.find("window") != std::string::npos)    return glm::vec3(0.60f, 0.75f, 0.85f);
        if (lower.find("concrete") != std::string::npos)  return glm::vec3(0.65f, 0.63f, 0.60f);
        if (lower.find("slab") != std::string::npos)      return glm::vec3(0.60f, 0.58f, 0.55f);
        if (lower.find("road") != std::string::npos)      return glm::vec3(0.30f, 0.30f, 0.28f);
        if (lower.find("asphalt") != std::string::npos)   return glm::vec3(0.25f, 0.25f, 0.25f);
        if (lower.find("grass") != std::string::npos)     return glm::vec3(0.20f, 0.50f, 0.12f);
        if (lower.find("metal") != std::string::npos)     return glm::vec3(0.35f, 0.35f, 0.38f);
        if (lower.find("lamp") != std::string::npos)      return glm::vec3(0.90f, 0.85f, 0.60f);
        return glm::vec3(0.7f, 0.7f, 0.7f);
    };

    std::vector<glm::vec3> matColors;
    matColors.reserve(materials.size());
    for (const auto& mat : materials) {
        glm::vec3 kd(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]);
        bool isDefaultGrey = (std::abs(kd.r - 0.8f) < 0.01f && std::abs(kd.g - 0.8f) < 0.01f && std::abs(kd.b - 0.8f) < 0.01f);
        if (isDefaultGrey) matColors.push_back(getSemanticColor(mat.name));
        else matColors.push_back(kd);
    }

    for (const auto& shape : t_shapes) {
        OBJShape outShape;
        outShape.name = shape.name;
        
        const auto& mesh = shape.mesh;
        const size_t numIndices = mesh.indices.size();
        outShape.vertices.reserve(numIndices / 2);
        outShape.indices.reserve(numIndices);

        std::unordered_map<VertexKey, unsigned int, VertexKeyHash, VertexKeyEqual> uniqueVertices;

        for (size_t i = 0; i < numIndices; ++i) {
            const tinyobj::index_t& idx = mesh.indices[i];
            size_t faceIndex = i / 3;
            glm::vec3 faceColor(0.7f);
            if (faceIndex < mesh.material_ids.size()) {
                int matId = mesh.material_ids[faceIndex];
                if (matId >= 0 && static_cast<size_t>(matId) < matColors.size()) {
                    faceColor = matColors[matId];
                }
            }

            VertexKey key;
            key.v = idx.vertex_index;
            key.n = idx.normal_index;

            auto [it, inserted] = uniqueVertices.try_emplace(key, static_cast<unsigned int>(outShape.vertices.size()));

            if (inserted) {
                MeshVertex vertex;
                if (key.v >= 0) {
                    vertex.position = glm::vec3(attrib.vertices[3 * key.v + 0], attrib.vertices[3 * key.v + 1], attrib.vertices[3 * key.v + 2]);
                }
                if (key.n >= 0) {
                    vertex.normal = glm::vec3(attrib.normals[3 * key.n + 0], attrib.normals[3 * key.n + 1], attrib.normals[3 * key.n + 2]);
                } else {
                    vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
                }
                if (idx.texcoord_index >= 0 && static_cast<size_t>(idx.texcoord_index * 2 + 1) < attrib.texcoords.size()) {
                    vertex.uv = glm::vec2(
                        attrib.texcoords[2 * idx.texcoord_index + 0],
                        attrib.texcoords[2 * idx.texcoord_index + 1]
                    );
                } else {
                    vertex.uv = glm::vec2(0.0f, 0.0f);
                }
                vertex.color = faceColor;
                outShape.vertices.push_back(vertex);
            }
            outShape.indices.push_back(it->second);
        }

        if (!outShape.vertices.empty()) {
            shapes.push_back(std::move(outShape));
        }
    }

    return !shapes.empty();
}
