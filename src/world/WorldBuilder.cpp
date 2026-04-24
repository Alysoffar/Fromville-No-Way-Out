#include "WorldBuilder.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cmath>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include "renderer/Camera.h"
#include "renderer/Renderer.h"
#include "renderer/Shader.h"
#include "world/DayNightCycle.h"
#include "world/RoadNetwork.h"
#include "world/Terrain.h"
#include "renderer/Texture.h"

namespace {

template <typename PixelFunc>
GLuint CreatePatternTexture(int width, int height, PixelFunc&& pixelFunc) {
    std::vector<unsigned char> pixels(static_cast<size_t>(width * height * 4), 0);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const glm::vec4 color = pixelFunc(x, y);
            const size_t index = static_cast<size_t>((y * width + x) * 4);
            pixels[index + 0] = static_cast<unsigned char>(glm::clamp(color.r, 0.0f, 1.0f) * 255.0f);
            pixels[index + 1] = static_cast<unsigned char>(glm::clamp(color.g, 0.0f, 1.0f) * 255.0f);
            pixels[index + 2] = static_cast<unsigned char>(glm::clamp(color.b, 0.0f, 1.0f) * 255.0f);
            pixels[index + 3] = static_cast<unsigned char>(glm::clamp(color.a, 0.0f, 1.0f) * 255.0f);
        }
    }

    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    return texture;
}

float Fract(float value) {
    return value - std::floor(value);
}

float SmoothStep(float edge0, float edge1, float value) {
    const float t = glm::clamp((value - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

GLuint GetWallTexture() {
    static GLuint texture = 0;
    if (texture != 0) {
        return texture;
    }

    texture = Texture::Load("assets/textures/Old_Medieval_Shack_House_/shack_main_diff.png");
    if (texture == 0) {
        texture = Texture::Load("assets/textures/house_1_v2 (1).png");
    }
    if (texture == 0) {
        // Fallback to procedural if file missing
        texture = CreatePatternTexture(256, 256, [](int x, int y) {
            const float u = static_cast<float>(x) / 255.0f;
            const float v = static_cast<float>(y) / 255.0f;
            const float rowShift = (static_cast<int>(v * 22.0f) % 2 == 0) ? 0.0f : 0.045f;
            const float board = std::abs(Fract((u + rowShift) * 8.0f) - 0.5f);
            const float seam = SmoothStep(0.42f, 0.49f, board);
            const float grime = glm::clamp(0.18f + 0.55f * std::sin(u * 12.0f + v * 9.0f), 0.0f, 1.0f);
            glm::vec3 color(0.82f, 0.80f, 0.72f);
            color *= 0.86f + seam * 0.10f;
            color -= grime * 0.05f;
            color += glm::vec3(0.02f, 0.01f, 0.00f) * static_cast<float>((x + y) % 5) * 0.01f;
            return glm::vec4(color, 1.0f);
        });
    }
    return texture;
}

GLuint GetRoofTexture() {
    static GLuint texture = 0;
    if (texture != 0) {
        return texture;
    }

    texture = CreatePatternTexture(256, 256, [](int x, int y) {
        const float u = static_cast<float>(x) / 255.0f;
        const float v = static_cast<float>(y) / 255.0f;
        const float row = std::floor(v * 28.0f);
        const float offset = (static_cast<int>(row) % 2 == 0) ? 0.0f : 0.5f;
        const float shingle = std::abs(Fract(u * 18.0f + offset) - 0.5f);
        const float ridge = SmoothStep(0.44f, 0.5f, std::abs(u - 0.5f));
        const float noise = 0.5f + 0.5f * std::sin(u * 35.0f + v * 23.0f + static_cast<float>((x * 17 + y * 31) % 13));
        glm::vec3 color(0.19f, 0.19f, 0.18f);
        color *= 0.82f + (1.0f - shingle) * 0.14f;
        color -= ridge * 0.08f;
        color += noise * 0.03f;
        return glm::vec4(color, 1.0f);
    });
    return texture;
}

GLuint GetTrimTexture() {
    static GLuint texture = 0;
    if (texture != 0) {
        return texture;
    }

    texture = CreatePatternTexture(128, 128, [](int x, int y) {
        const float u = static_cast<float>(x) / 127.0f;
        const float v = static_cast<float>(y) / 127.0f;
        const float grain = 0.5f + 0.5f * std::sin(u * 22.0f + v * 8.0f + static_cast<float>((x + y) % 11));
        glm::vec3 color(0.22f, 0.19f, 0.16f);
        color += grain * 0.05f;
        color -= glm::vec3(v * 0.03f);
        return glm::vec4(color, 1.0f);
    });
    return texture;
}

GLuint GetSignTexture() {
    static GLuint texture = 0;
    if (texture != 0) {
        return texture;
    }

    texture = CreatePatternTexture(128, 128, [](int x, int y) {
        const float u = static_cast<float>(x) / 127.0f;
        const float v = static_cast<float>(y) / 127.0f;
        const float worn = 0.5f + 0.5f * std::sin(u * 14.0f + v * 9.0f);
        glm::vec3 color(0.58f, 0.45f, 0.26f);
        color *= 0.88f + worn * 0.08f;
        return glm::vec4(color, 1.0f);
    });
    return texture;
}

GLuint GetStreetTexture() {
    static GLuint texture = 0;
    if (texture != 0) {
        return texture;
    }

    texture = CreatePatternTexture(256, 256, [](int x, int y) {
        const float u = static_cast<float>(x) / 255.0f;
        const float v = static_cast<float>(y) / 255.0f;
        const float noise = 0.5f + 0.5f * std::sin(u * 28.0f + v * 21.0f + static_cast<float>((x * 7 + y * 13) % 19));
        const float crack = std::abs(Fract(u * 7.0f + v * 5.0f) - 0.5f);
        glm::vec3 color(0.20f, 0.20f, 0.19f);
        color *= 0.84f + noise * 0.14f;
        color -= glm::vec3(crack * 0.04f);
        return glm::vec4(color, 1.0f);
    });
    return texture;
}

glm::vec3 SafeNormal(const glm::vec3& value) {
    const float length = glm::length(value);
    if (length < 1e-5f) {
        return glm::vec3(0.0f, 1.0f, 0.0f);
    }
    return value / length;
}

void PushQuad(std::vector<WorldSceneVertex>& verts,
              std::vector<unsigned int>& inds,
              const glm::vec3& a,
              const glm::vec3& b,
              const glm::vec3& c,
              const glm::vec3& d,
              const glm::vec3& normal) {
    const unsigned int base = static_cast<unsigned int>(verts.size());
    verts.push_back({a, normal, glm::vec2(0.0f, 0.0f)});
    verts.push_back({b, normal, glm::vec2(1.0f, 0.0f)});
    verts.push_back({c, normal, glm::vec2(1.0f, 1.0f)});
    verts.push_back({d, normal, glm::vec2(0.0f, 1.0f)});

    inds.push_back(base + 0);
    inds.push_back(base + 1);
    inds.push_back(base + 2);
    inds.push_back(base + 0);
    inds.push_back(base + 2);
    inds.push_back(base + 3);
}

void PushTriangle(std::vector<WorldSceneVertex>& verts,
                  std::vector<unsigned int>& inds,
                  const glm::vec3& a,
                  const glm::vec3& b,
                  const glm::vec3& c) {
    const glm::vec3 normal = SafeNormal(glm::cross(b - a, c - a));
    const unsigned int base = static_cast<unsigned int>(verts.size());
    verts.push_back({a, normal, glm::vec2(0.0f, 0.0f)});
    verts.push_back({b, normal, glm::vec2(1.0f, 0.0f)});
    verts.push_back({c, normal, glm::vec2(0.5f, 1.0f)});
    inds.push_back(base + 0);
    inds.push_back(base + 1);
    inds.push_back(base + 2);
}


void PushBox(std::vector<WorldSceneVertex>& verts,
             std::vector<unsigned int>& inds,
             const glm::vec3& center,
             const glm::vec3& size) {
    const glm::vec3 h = size * 0.5f;

    const glm::vec3 p000 = center + glm::vec3(-h.x, -h.y, -h.z);
    const glm::vec3 p001 = center + glm::vec3(-h.x, -h.y,  h.z);
    const glm::vec3 p010 = center + glm::vec3(-h.x,  h.y, -h.z);
    const glm::vec3 p011 = center + glm::vec3(-h.x,  h.y,  h.z);
    const glm::vec3 p100 = center + glm::vec3( h.x, -h.y, -h.z);
    const glm::vec3 p101 = center + glm::vec3( h.x, -h.y,  h.z);
    const glm::vec3 p110 = center + glm::vec3( h.x,  h.y, -h.z);
    const glm::vec3 p111 = center + glm::vec3( h.x,  h.y,  h.z);

    PushQuad(verts, inds, p001, p101, p111, p011, glm::vec3(0.0f, 0.0f, 1.0f));
    PushQuad(verts, inds, p100, p000, p010, p110, glm::vec3(0.0f, 0.0f, -1.0f));
    PushQuad(verts, inds, p000, p001, p011, p010, glm::vec3(-1.0f, 0.0f, 0.0f));
    PushQuad(verts, inds, p101, p100, p110, p111, glm::vec3(1.0f, 0.0f, 0.0f));
    PushQuad(verts, inds, p010, p011, p111, p110, glm::vec3(0.0f, 1.0f, 0.0f));
    PushQuad(verts, inds, p100, p101, p001, p000, glm::vec3(0.0f, -1.0f, 0.0f));
}

void PushCylinder(std::vector<WorldSceneVertex>& verts,
                  std::vector<unsigned int>& inds,
                  const glm::vec3& center,
                  float radius,
                  float height,
                  int sides,
                  bool capTop = true,
                  bool capBottom = true) {
    sides = std::max(6, sides);
    const float halfHeight = height * 0.5f;

    for (int i = 0; i < sides; ++i) {
        const float a0 = glm::two_pi<float>() * static_cast<float>(i) / static_cast<float>(sides);
        const float a1 = glm::two_pi<float>() * static_cast<float>(i + 1) / static_cast<float>(sides);

        const glm::vec3 p0 = center + glm::vec3(std::cos(a0) * radius, -halfHeight, std::sin(a0) * radius);
        const glm::vec3 p1 = center + glm::vec3(std::cos(a1) * radius, -halfHeight, std::sin(a1) * radius);
        const glm::vec3 p2 = center + glm::vec3(std::cos(a1) * radius,  halfHeight, std::sin(a1) * radius);
        const glm::vec3 p3 = center + glm::vec3(std::cos(a0) * radius,  halfHeight, std::sin(a0) * radius);

        const glm::vec3 n0 = SafeNormal(glm::vec3(std::cos(a0), 0.0f, std::sin(a0)));
        const glm::vec3 n1 = SafeNormal(glm::vec3(std::cos(a1), 0.0f, std::sin(a1)));
        const unsigned int base = static_cast<unsigned int>(verts.size());
        verts.push_back({p0, n0, glm::vec2(0.0f, 0.0f)});
        verts.push_back({p1, n1, glm::vec2(1.0f, 0.0f)});
        verts.push_back({p2, n1, glm::vec2(1.0f, 1.0f)});
        verts.push_back({p3, n0, glm::vec2(0.0f, 1.0f)});
        inds.push_back(base + 0);
        inds.push_back(base + 1);
        inds.push_back(base + 2);
        inds.push_back(base + 2);
        inds.push_back(base + 3);
        inds.push_back(base + 0);

        if (capTop) {
            PushTriangle(verts, inds, center + glm::vec3(0.0f, halfHeight, 0.0f), p3, p2);
        }
        if (capBottom) {
            PushTriangle(verts, inds, center + glm::vec3(0.0f, -halfHeight, 0.0f), p1, p0);
        }
    }
}

void PushCone(std::vector<WorldSceneVertex>& verts,
              std::vector<unsigned int>& inds,
              const glm::vec3& center,
              float radius,
              float height,
              int sides) {
    sides = std::max(6, sides);
    const glm::vec3 apex = center + glm::vec3(0.0f, height * 0.5f, 0.0f);
    const glm::vec3 baseCenter = center - glm::vec3(0.0f, height * 0.5f, 0.0f);

    for (int i = 0; i < sides; ++i) {
        const float a0 = glm::two_pi<float>() * static_cast<float>(i) / static_cast<float>(sides);
        const float a1 = glm::two_pi<float>() * static_cast<float>(i + 1) / static_cast<float>(sides);
        const glm::vec3 p0 = baseCenter + glm::vec3(std::cos(a0) * radius, 0.0f, std::sin(a0) * radius);
        const glm::vec3 p1 = baseCenter + glm::vec3(std::cos(a1) * radius, 0.0f, std::sin(a1) * radius);

        PushTriangle(verts, inds, p0, p1, apex);
        PushTriangle(verts, inds, baseCenter, p1, p0);
    }
}

void PushGabledRoof(std::vector<WorldSceneVertex>& verts,
                    std::vector<unsigned int>& inds,
                    const glm::vec3& center,
                    float width,
                    float depth,
                    float eaveHeight,
                    float ridgeHeight,
                    float overhang) {
    const float halfW = width * 0.5f + overhang;
    const float halfD = depth * 0.5f + overhang;

    const glm::vec3 frontLeft  = center + glm::vec3(-halfW, eaveHeight,  halfD);
    const glm::vec3 frontRight = center + glm::vec3( halfW, eaveHeight,  halfD);
    const glm::vec3 backLeft   = center + glm::vec3(-halfW, eaveHeight, -halfD);
    const glm::vec3 backRight  = center + glm::vec3( halfW, eaveHeight, -halfD);
    const glm::vec3 ridgeFront = center + glm::vec3(0.0f, ridgeHeight,  halfD);
    const glm::vec3 ridgeBack  = center + glm::vec3(0.0f, ridgeHeight, -halfD);

    PushQuad(verts, inds, backLeft, frontLeft, ridgeFront, ridgeBack, SafeNormal(glm::cross(frontLeft - backLeft, ridgeBack - backLeft)));
    PushQuad(verts, inds, frontRight, backRight, ridgeBack, ridgeFront, SafeNormal(glm::cross(backRight - frontRight, ridgeFront - frontRight)));
    PushTriangle(verts, inds, frontLeft, frontRight, ridgeFront);
    PushTriangle(verts, inds, backRight, backLeft, ridgeBack);
}

void PushThinBeam(std::vector<WorldSceneVertex>& verts,
                  std::vector<unsigned int>& inds,
                  const glm::vec3& start,
                  const glm::vec3& end,
                  float thickness) {
    const glm::vec3 dir = end - start;
    const float length = glm::length(dir);
    if (length < 1e-4f) {
        return;
    }

    const glm::vec3 forward = dir / length;
    const glm::vec3 right = SafeNormal(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    const glm::vec3 up = SafeNormal(glm::cross(right, forward));
    const glm::vec3 center = (start + end) * 0.5f;
    const glm::vec3 half = glm::vec3(thickness, thickness, length * 0.5f);

    const glm::mat3 basis(right, up, forward);
    const auto makePoint = [&](float x, float y, float z) {
        return center + basis * glm::vec3(x, y, z);
    };

    const glm::vec3 p000 = makePoint(-half.x, -half.y, -half.z);
    const glm::vec3 p001 = makePoint(-half.x, -half.y,  half.z);
    const glm::vec3 p010 = makePoint(-half.x,  half.y, -half.z);
    const glm::vec3 p011 = makePoint(-half.x,  half.y,  half.z);
    const glm::vec3 p100 = makePoint( half.x, -half.y, -half.z);
    const glm::vec3 p101 = makePoint( half.x, -half.y,  half.z);
    const glm::vec3 p110 = makePoint( half.x,  half.y, -half.z);
    const glm::vec3 p111 = makePoint( half.x,  half.y,  half.z);

    PushQuad(verts, inds, p101, p001, p011, p111, SafeNormal(glm::cross(p001 - p101, p111 - p101)));
    PushQuad(verts, inds, p000, p100, p110, p010, SafeNormal(glm::cross(p100 - p000, p010 - p000)));
    PushQuad(verts, inds, p001, p000, p010, p011, SafeNormal(glm::cross(p000 - p001, p011 - p001)));
    PushQuad(verts, inds, p100, p101, p111, p110, SafeNormal(glm::cross(p101 - p100, p110 - p100)));
    PushQuad(verts, inds, p010, p110, p111, p011, SafeNormal(glm::cross(p110 - p010, p011 - p010)));
    PushQuad(verts, inds, p001, p101, p100, p000, SafeNormal(glm::cross(p101 - p001, p000 - p001)));
}

} // namespace

WorldBuilder::WorldBuilder(NavMesh* navMesh, TalismanSystem* talismans)
    : nav(navMesh), talismanSystem(talismans) {}

WorldBuilder::~WorldBuilder() {
    ClearSceneBatches();
}

void PushMedievalHouse(std::vector<WorldSceneVertex>& wallVerts,
                       std::vector<unsigned int>& wallInds,
                       std::vector<WorldSceneVertex>& accentVerts,
                       std::vector<unsigned int>& accentInds,
                       std::vector<WorldSceneVertex>& roofVerts,
                       std::vector<unsigned int>& roofInds,
                       glm::vec3 center,
                       glm::vec3 size) {
    // Base part (Stone)
    float baseHeight = size.y * 0.45f;
    PushBox(wallVerts, wallInds, center - glm::vec3(0.0f, size.y * 0.5f - baseHeight * 0.5f, 0.0f), glm::vec3(size.x, baseHeight, size.z));
    
    // Top part (Wood)
    float topHeight = size.y - baseHeight;
    PushBox(accentVerts, accentInds, center + glm::vec3(0.0f, size.y * 0.5f - topHeight * 0.5f, 0.0f), glm::vec3(size.x * 1.02f, topHeight, size.z * 1.02f));

    // Roof
    PushGabledRoof(roofVerts, roofInds, center + glm::vec3(0.0f, size.y * 0.5f, 0.0f), size.x * 1.1f, size.z * 1.1f, 0.0f, size.y * 0.4f, 0.3f);
}

void WorldBuilder::BuildAll() {
    if (!worldShader) {
        worldShader = std::make_unique<Shader>("ProceduralWorld");
    }

    try {
        worldShader->Load("assets/shaders/procedural_world.vert", "assets/shaders/procedural_world.frag");
        shaderReady = true;
        worldShader->Bind();
        worldShader->SetInt("baseTexture", 0);
        worldShader->SetInt("terrainGrassTexture", 0);
        worldShader->SetInt("terrainDirtTexture", 1);
        worldShader->SetInt("terrainRockTexture", 2);
        worldShader->SetBool("useTexture", false);
        worldShader->SetBool("terrainMaterial", false);
        worldShader->SetFloat("terrainTextureScale", 0.065f);
        worldShader->Unbind();
    } catch (const std::exception& e) {
        shaderReady = false;
        std::cerr << "World shader load failed: " << e.what() << '\n';
    }

    ClearSceneBatches();
    BuildTerrain();
    BuildRoadNetwork();
    BuildColonyHouse();
    BuildSurvivorCabins();
    BuildLandmarkBuildings();
    BuildForestInterior();
    BuildForestBoundary();
    BuildDistantRidges();
    BuildWhiteTreeGroves();
    BuildBigRedMarker();
    BuildAtmosphericProps();
    BuildBottleTreeShrine();
    BuildTownFacades();
    BuildFencedLots();
    BuildTunnelEntrances();
    BuildSheriffStation();
    BuildWaterTower();
    BuildStoneCircles();
    BuildSky();
    BuildFog();
}

void WorldBuilder::DrawAll(Renderer& renderer, Camera& camera, DayNightCycle& dayNight) {
    (void)dayNight;

    if (!shaderReady || !worldShader) {
        return;
    }

    worldShader->Bind();
    worldShader->SetMat4("view", camera.GetViewMatrix());
    worldShader->SetMat4("projection", camera.GetProjectionMatrix(renderer.GetAspectRatio()));

    if (terrain) {
        terrain->Draw(*worldShader);
        worldShader->SetBool("terrainMaterial", false);
    }
    if (roads) {
        roads->Draw(*worldShader);
    }
    for (const WorldSceneBatch& batch : sceneBatches) {
        if (!batch.vao || batch.indexCount <= 0) {
            continue;
        }

        worldShader->SetMat4("model", glm::mat4(1.0f));
        worldShader->SetVec3("albedoColor", batch.albedo);
        worldShader->SetFloat("roughness", batch.roughness);
        worldShader->SetFloat("metalness", batch.metalness);
        worldShader->SetVec3("emission", batch.emission);
        worldShader->SetBool("useTexture", batch.textureID != 0);
        if (batch.textureID != 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, batch.textureID);
        }

        glBindVertexArray(batch.vao);
        glDrawElements(GL_TRIANGLES, batch.indexCount, GL_UNSIGNED_INT, nullptr);
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    worldShader->Unbind();
}

float WorldBuilder::GetHeightAt(float x, float z) const {
    if (!terrain) {
        return 0.0f;
    }
    return terrain->GetHeightAt(x, z);
}

void WorldBuilder::BuildTerrain() {
    terrain = std::make_unique<Terrain>(220, 220, 4.0f);
    terrain->Generate();
}

void WorldBuilder::BuildRoadNetwork() {
    roads = std::make_unique<RoadNetwork>();
    if (terrain) {
        roads->Build(*terrain);
    }
}

void WorldBuilder::BuildColonyHouse() {
    if (!terrain) {
        return;
    }

    std::vector<WorldSceneVertex> foundationVerts;
    std::vector<unsigned int> foundationInds;
    std::vector<WorldSceneVertex> wallVerts;
    std::vector<unsigned int> wallInds;
    std::vector<WorldSceneVertex> roofVerts;
    std::vector<unsigned int> roofInds;
    std::vector<WorldSceneVertex> porchVerts;
    std::vector<unsigned int> porchInds;

    const glm::vec3 houseCenter(-34.0f, GetHeightAt(-34.0f, -22.0f), -22.0f);
    const float wallHeight = 7.6f;

    PushBox(foundationVerts, foundationInds, houseCenter + glm::vec3(0.0f, 0.45f, 0.0f), glm::vec3(25.0f, 0.9f, 14.0f));
    PushBox(wallVerts, wallInds, houseCenter + glm::vec3(0.0f, wallHeight * 0.5f + 0.45f, 0.0f), glm::vec3(23.0f, wallHeight, 12.0f));
    PushGabledRoof(roofVerts, roofInds, houseCenter + glm::vec3(0.0f, wallHeight + 0.45f, 0.0f), 24.0f, 13.0f, 0.2f, 4.8f, 0.7f);
    PushBox(porchVerts, porchInds, houseCenter + glm::vec3(0.0f, 1.1f, 8.0f), glm::vec3(26.5f, 0.35f, 4.5f));
    PushBox(porchVerts, porchInds, houseCenter + glm::vec3(0.0f, 4.6f, 8.0f), glm::vec3(26.5f, 0.25f, 4.7f));

    for (int i = -5; i <= 5; i += 2) {
        PushBox(porchVerts, porchInds, houseCenter + glm::vec3(static_cast<float>(i) * 2.0f, 2.9f, 8.8f), glm::vec3(0.28f, 3.6f, 0.28f));
    }

    PushBox(porchVerts, porchInds, houseCenter + glm::vec3(0.0f, 0.55f, 10.3f), glm::vec3(4.4f, 0.18f, 1.0f));
    PushBox(porchVerts, porchInds, houseCenter + glm::vec3(0.0f, 0.28f, 11.3f), glm::vec3(3.6f, 0.18f, 1.0f));
    PushBox(porchVerts, porchInds, houseCenter + glm::vec3(0.0f, 0.02f, 12.1f), glm::vec3(2.8f, 0.18f, 0.9f));

    UploadBatch(foundationVerts, foundationInds, glm::vec3(0.40f, 0.39f, 0.37f), 0.92f, GetStreetTexture());
    UploadBatch(wallVerts, wallInds, glm::vec3(0.83f, 0.81f, 0.74f), 0.88f, GetWallTexture());
    UploadBatch(roofVerts, roofInds, glm::vec3(0.22f, 0.22f, 0.21f), 0.86f, GetRoofTexture());
    UploadBatch(porchVerts, porchInds, glm::vec3(0.55f, 0.50f, 0.43f), 0.84f, GetTrimTexture());
}

void WorldBuilder::BuildSurvivorCabins() {
    if (!terrain) {
        return;
    }

    std::vector<WorldSceneVertex> wallVerts;
    std::vector<unsigned int> wallInds;
    std::vector<WorldSceneVertex> roofVerts;
    std::vector<unsigned int> roofInds;
    std::vector<WorldSceneVertex> propVerts;
    std::vector<unsigned int> propInds;

    struct CabinData {
        glm::vec2 pos;
        glm::vec3 size;
    };

    const std::array<CabinData, 12> cabins = {{
        {{-118.0f, -86.0f}, glm::vec3(7.0f, 4.6f, 6.0f)},
        {{-98.0f, -62.0f}, glm::vec3(6.4f, 4.2f, 5.8f)},
        {{-148.0f, -110.0f}, glm::vec3(6.0f, 3.8f, 4.8f)},
        {{-46.0f, 108.0f}, glm::vec3(6.5f, 4.0f, 5.2f)},
        {{-18.0f, 126.0f}, glm::vec3(6.2f, 3.8f, 5.0f)},
        {{ 18.0f, 124.0f}, glm::vec3(6.4f, 3.9f, 5.0f)},
        {{ 86.0f,  78.0f}, glm::vec3(7.6f, 4.8f, 6.2f)},
        {{108.0f, 112.0f}, glm::vec3(6.8f, 4.0f, 5.2f)},
        {{124.0f, 124.0f}, glm::vec3(5.8f, 3.6f, 4.6f)},
        {{ 44.0f, -92.0f}, glm::vec3(7.2f, 4.4f, 6.0f)},
        {{ 96.0f, -108.0f}, glm::vec3(6.6f, 4.2f, 5.8f)},
        {{136.0f, -62.0f}, glm::vec3(6.0f, 3.8f, 4.8f)},
    }};

    for (std::size_t i = 0; i < cabins.size(); ++i) {
        const CabinData& cabin = cabins[i];
        const float ground = GetHeightAt(cabin.pos.x, cabin.pos.y);
        const glm::vec3 center(cabin.pos.x, ground + cabin.size.y * 0.5f + 0.25f, cabin.pos.y);
        PushBox(wallVerts, wallInds, center, cabin.size);
        PushGabledRoof(roofVerts, roofInds, center + glm::vec3(0.0f, cabin.size.y * 0.5f, 0.0f), cabin.size.x + 0.4f, cabin.size.z + 0.3f, 0.0f, 2.0f, 0.4f);
        PushBox(propVerts, propInds, glm::vec3(cabin.pos.x + cabin.size.x * 0.45f, ground + 0.42f, cabin.pos.y + cabin.size.z * 0.55f), glm::vec3(1.0f, 0.84f, 1.0f));
        PushBox(propVerts, propInds, glm::vec3(cabin.pos.x - cabin.size.x * 0.55f, ground + 0.32f, cabin.pos.y - cabin.size.z * 0.62f), glm::vec3(1.4f, 0.65f, 0.8f));
        PushCylinder(propVerts, propInds, glm::vec3(cabin.pos.x + cabin.size.x * 0.65f, ground + 0.55f, cabin.pos.y - cabin.size.z * 0.7f), 0.18f, 1.1f, 10, true, true);
    }

    UploadBatch(wallVerts, wallInds, glm::vec3(0.58f, 0.52f, 0.45f), 0.87f, GetWallTexture());
    UploadBatch(roofVerts, roofInds, glm::vec3(0.26f, 0.24f, 0.24f), 0.9f, GetRoofTexture());
    UploadBatch(propVerts, propInds, glm::vec3(0.32f, 0.28f, 0.23f), 0.82f, GetTrimTexture());
}

void WorldBuilder::BuildLandmarkBuildings() {
    if (!terrain) {
        return;
    }

    std::vector<WorldSceneVertex> wallVerts;
    std::vector<unsigned int> wallInds;
    std::vector<WorldSceneVertex> roofVerts;
    std::vector<unsigned int> roofInds;
    std::vector<WorldSceneVertex> accentVerts;
    std::vector<unsigned int> accentInds;

    const float churchGround = GetHeightAt(-82.0f, 54.0f);
    const glm::vec3 churchCenter(-82.0f, churchGround + 4.2f, 54.0f);
    PushBox(wallVerts, wallInds, churchCenter, glm::vec3(13.0f, 8.4f, 18.0f));
    PushGabledRoof(roofVerts, roofInds, churchCenter + glm::vec3(0.0f, 4.2f, 0.0f), 13.8f, 18.6f, 0.0f, 3.2f, 0.5f);
    PushBox(accentVerts, accentInds, churchCenter + glm::vec3(0.0f, 6.5f, 10.0f), glm::vec3(3.2f, 5.0f, 3.2f));
    PushCone(accentVerts, accentInds, churchCenter + glm::vec3(0.0f, 10.2f, 10.0f), 1.6f, 3.6f, 8);
    PushThinBeam(accentVerts, accentInds, churchCenter + glm::vec3(0.0f, 12.2f, 10.0f), churchCenter + glm::vec3(0.0f, 13.4f, 10.0f), 0.08f);
    PushThinBeam(accentVerts, accentInds, churchCenter + glm::vec3(-0.4f, 12.8f, 10.0f), churchCenter + glm::vec3(0.4f, 12.8f, 10.0f), 0.08f);

    const float dinerGround = GetHeightAt(-8.0f, 84.0f);
    const glm::vec3 dinerCenter(-8.0f, dinerGround + 3.0f, 84.0f);
    PushBox(wallVerts, wallInds, dinerCenter, glm::vec3(20.0f, 6.0f, 11.0f));
    PushBox(accentVerts, accentInds, dinerCenter + glm::vec3(0.0f, 1.2f, 5.8f), glm::vec3(18.0f, 1.8f, 0.3f));
    PushBox(accentVerts, accentInds, dinerCenter + glm::vec3(0.0f, 3.8f, 5.8f), glm::vec3(14.0f, 0.45f, 0.25f));
    PushBox(roofVerts, roofInds, dinerCenter + glm::vec3(0.0f, 3.2f, 0.0f), glm::vec3(20.8f, 0.35f, 11.8f));

    const float clinicGround = GetHeightAt(-18.0f, -12.0f);
    const glm::vec3 clinicCenter(-18.0f, clinicGround + 3.5f, -12.0f);
    PushMedievalHouse(wallVerts, wallInds, accentVerts, accentInds, roofVerts, roofInds, clinicCenter, glm::vec3(18.0f, 7.0f, 12.0f));

    const float motelGround = GetHeightAt(86.0f, -18.0f);
    const glm::vec3 motelCenter(86.0f, motelGround + 3.2f, -18.0f);
    PushMedievalHouse(wallVerts, wallInds, accentVerts, accentInds, roofVerts, roofInds, motelCenter, glm::vec3(28.0f, 6.4f, 12.0f));

    const float gasGround = GetHeightAt(78.0f, 74.0f);
    const glm::vec3 gasCenter(78.0f, gasGround + 2.7f, 74.0f);
    PushMedievalHouse(wallVerts, wallInds, accentVerts, accentInds, roofVerts, roofInds, gasCenter, glm::vec3(16.0f, 5.4f, 13.0f));

    const float postGround = GetHeightAt(-34.0f, 72.0f);
    const glm::vec3 postCenter(-34.0f, postGround + 2.8f, 72.0f);
    PushMedievalHouse(wallVerts, wallInds, accentVerts, accentInds, roofVerts, roofInds, postCenter, glm::vec3(11.0f, 5.6f, 10.0f));

    const float storeGround = GetHeightAt(-12.0f, 98.0f);
    const glm::vec3 storeCenter(-12.0f, storeGround + 2.5f, 98.0f);
    PushMedievalHouse(wallVerts, wallInds, accentVerts, accentInds, roofVerts, roofInds, storeCenter, glm::vec3(14.0f, 5.0f, 9.0f));

    const float schoolGround = GetHeightAt(-58.0f, 48.0f);
    const glm::vec3 schoolCenter(-58.0f, schoolGround + 2.7f, 48.0f);
    PushMedievalHouse(wallVerts, wallInds, accentVerts, accentInds, roofVerts, roofInds, schoolCenter, glm::vec3(16.0f, 5.4f, 11.0f));

    const float apartmentGround = GetHeightAt(104.0f, 54.0f);
    const glm::vec3 apartmentCenter(104.0f, apartmentGround + 4.6f, 54.0f);
    PushMedievalHouse(wallVerts, wallInds, accentVerts, accentInds, roofVerts, roofInds, apartmentCenter, glm::vec3(18.0f, 9.2f, 12.0f));

    UploadBatch(wallVerts, wallInds, glm::vec3(0.66f, 0.62f, 0.56f), 0.84f, GetWallTexture());
    UploadBatch(roofVerts, roofInds, glm::vec3(0.24f, 0.23f, 0.22f), 0.86f, GetRoofTexture());
    UploadBatch(accentVerts, accentInds, glm::vec3(0.74f, 0.68f, 0.50f), 0.72f, GetTrimTexture());
}

void WorldBuilder::BuildForestInterior() {
    if (!terrain) {
        return;
    }

    std::vector<WorldSceneVertex> trunkVerts;
    std::vector<unsigned int> trunkInds;
    std::vector<WorldSceneVertex> foliageVerts;
    std::vector<unsigned int> foliageInds;

    constexpr int interiorTreeCount = 72;
    for (int i = 0; i < interiorTreeCount; ++i) {
        const float angle = glm::two_pi<float>() * static_cast<float>(i) / static_cast<float>(interiorTreeCount) + std::sin(static_cast<float>(i) * 1.19f) * 0.18f;
        const float radius = 118.0f + std::fmod(static_cast<float>(i) * 23.7f, 82.0f);
        const float x = std::cos(angle) * radius;
        const float z = std::sin(angle) * radius * 0.92f;
        if (std::abs(x) < 92.0f && std::abs(z) < 92.0f) {
            continue;
        }

        const float ground = GetHeightAt(x, z);
        const float trunkHeight = 6.2f + std::fmod(static_cast<float>(i) * 0.87f, 4.8f);
        const float trunkRadius = 0.20f + std::fmod(static_cast<float>(i) * 0.14f, 0.10f);
        PushCylinder(trunkVerts, trunkInds, glm::vec3(x, ground + trunkHeight * 0.5f, z), trunkRadius, trunkHeight, 9, true, true);

        if (i % 4 == 0) {
            PushBox(foliageVerts, foliageInds, glm::vec3(x, ground + trunkHeight + 1.7f, z), glm::vec3(3.2f, 2.6f, 3.0f));
        } else {
            PushCone(foliageVerts, foliageInds, glm::vec3(x, ground + trunkHeight + 1.8f, z), 2.2f, 3.8f, 8);
            PushCone(foliageVerts, foliageInds, glm::vec3(x, ground + trunkHeight + 3.9f, z), 1.6f, 3.0f, 8);
        }
    }

    UploadBatch(trunkVerts, trunkInds, glm::vec3(0.22f, 0.15f, 0.10f), 0.95f);
    UploadBatch(foliageVerts, foliageInds, glm::vec3(0.12f, 0.24f, 0.10f), 0.96f);
}

void WorldBuilder::BuildForestBoundary() {
    if (!terrain) {
        return;
    }

    std::vector<WorldSceneVertex> trunkVerts;
    std::vector<unsigned int> trunkInds;
    std::vector<WorldSceneVertex> foliageVerts;
    std::vector<unsigned int> foliageInds;

    constexpr int treeCount = 260;
    for (int i = 0; i < treeCount; ++i) {
        const float angle = glm::two_pi<float>() * static_cast<float>(i) / static_cast<float>(treeCount);
        const float layer = static_cast<float>(i % 6);
        const float radius = 286.0f + layer * 18.0f + 58.0f * std::sin(static_cast<float>(i) * 1.37f) + 24.0f * std::cos(static_cast<float>(i) * 0.61f);
        const float x = std::cos(angle) * radius;
        const float z = std::sin(angle) * radius;
        const float ground = GetHeightAt(x, z);
        const float trunkHeight = 8.0f + std::fmod(static_cast<float>(i) * 1.93f, 9.0f) + (layer * 0.35f);
        const float trunkRadius = 0.24f + std::fmod(static_cast<float>(i) * 0.47f, 0.22f);

        PushCylinder(trunkVerts, trunkInds, glm::vec3(x, ground + trunkHeight * 0.5f, z), trunkRadius, trunkHeight, 10, true, true);

        if (i % 5 == 0) {
            PushBox(foliageVerts, foliageInds, glm::vec3(x, ground + trunkHeight + 2.8f, z), glm::vec3(4.8f, 4.0f, 4.4f));
            PushBox(foliageVerts, foliageInds, glm::vec3(x + 1.1f, ground + trunkHeight + 1.7f, z - 0.8f), glm::vec3(3.4f, 2.8f, 3.4f));
            PushCone(foliageVerts, foliageInds, glm::vec3(x - 0.9f, ground + trunkHeight + 4.8f, z + 0.5f), 2.1f, 3.8f, 10);
        } else {
            PushCone(foliageVerts, foliageInds, glm::vec3(x, ground + trunkHeight + 2.2f, z), 3.4f, 5.6f, 10);
            PushCone(foliageVerts, foliageInds, glm::vec3(x, ground + trunkHeight + 4.7f, z), 2.7f, 4.8f, 10);
            PushCone(foliageVerts, foliageInds, glm::vec3(x, ground + trunkHeight + 6.8f, z), 1.9f, 4.0f, 10);
        }
    }

    UploadBatch(trunkVerts, trunkInds, glm::vec3(0.18f, 0.12f, 0.08f), 0.94f);
    UploadBatch(foliageVerts, foliageInds, glm::vec3(0.11f, 0.24f, 0.10f), 0.95f);
}

void WorldBuilder::BuildDistantRidges() {
    if (!terrain) {
        return;
    }

    std::vector<WorldSceneVertex> ridgeVerts;
    std::vector<unsigned int> ridgeInds;
    std::vector<WorldSceneVertex> treeVerts;
    std::vector<unsigned int> treeInds;

    constexpr int ridgeCount = 60;
    for (int i = 0; i < ridgeCount; ++i) {
        const float angle = glm::two_pi<float>() * static_cast<float>(i) / static_cast<float>(ridgeCount);
        const float ring = 168.0f + 16.0f * std::sin(static_cast<float>(i) * 0.83f) + 12.0f * std::cos(static_cast<float>(i) * 1.37f);
        const float x = std::cos(angle) * ring;
        const float z = std::sin(angle) * ring * 0.93f;
        const float ground = GetHeightAt(x, z);

        const float ridgeHeight = 12.0f + std::fmod(static_cast<float>(i) * 3.7f, 10.0f);
        const float ridgeRadius = 14.0f + std::fmod(static_cast<float>(i) * 2.3f, 9.0f);
        PushCone(ridgeVerts, ridgeInds, glm::vec3(x, ground + ridgeHeight * 0.5f, z), ridgeRadius, ridgeHeight, 8);
        if (i % 2 == 0) {
            PushBox(ridgeVerts, ridgeInds, glm::vec3(x + 3.4f, ground + ridgeHeight * 0.28f, z - 1.8f), glm::vec3(8.0f, 4.2f, 6.8f));
        } else {
            PushCone(ridgeVerts, ridgeInds, glm::vec3(x - 2.6f, ground + ridgeHeight * 0.38f, z + 2.0f), ridgeRadius * 0.72f, ridgeHeight * 0.72f, 7);
        }

        const float forestRing = 148.0f + 16.0f * std::sin(static_cast<float>(i) * 1.19f);
        const float fx = std::cos(angle + 0.08f) * forestRing;
        const float fz = std::sin(angle + 0.08f) * forestRing * 0.96f;
        const float forestGround = GetHeightAt(fx, fz);
        const float trunkHeight = 12.0f + std::fmod(static_cast<float>(i) * 1.6f, 6.5f);
        PushCylinder(treeVerts, treeInds, glm::vec3(fx, forestGround + trunkHeight * 0.5f, fz), 0.22f + std::fmod(static_cast<float>(i) * 0.07f, 0.08f), trunkHeight, 8, true, true);
        PushCone(treeVerts, treeInds, glm::vec3(fx, forestGround + trunkHeight + 2.6f, fz), 3.2f + std::fmod(static_cast<float>(i) * 0.23f, 1.2f), 6.0f, 9);
        if (i % 4 == 0) {
            PushCone(treeVerts, treeInds, glm::vec3(fx + 1.0f, forestGround + trunkHeight + 5.0f, fz - 0.6f), 2.2f, 4.2f, 8);
        }
    }

    UploadBatch(ridgeVerts, ridgeInds, glm::vec3(0.22f, 0.24f, 0.20f), 0.92f);
    UploadBatch(treeVerts, treeInds, glm::vec3(0.10f, 0.20f, 0.09f), 0.95f);
}

void WorldBuilder::BuildWhiteTreeGroves() {
    if (!terrain) {
        return;
    }

    std::vector<WorldSceneVertex> trunkVerts;
    std::vector<unsigned int> trunkInds;
    std::vector<WorldSceneVertex> branchVerts;
    std::vector<unsigned int> branchInds;

    const std::array<glm::vec2, 2> groveCenters = {{
        {-146.0f, 92.0f},
        {118.0f, -102.0f},
    }};

    for (const glm::vec2& grove : groveCenters) {
        for (int i = 0; i < 7; ++i) {
            const float angle = glm::two_pi<float>() * static_cast<float>(i) / 7.0f;
            const float radius = 0.0f + static_cast<float>(i % 3) * 2.6f;
            const float x = grove.x + std::cos(angle) * radius;
            const float z = grove.y + std::sin(angle) * radius * 0.78f;
            const float ground = GetHeightAt(x, z);
            const float height = 7.8f + static_cast<float>(i % 3) * 1.3f;
            const float sway = (i % 2 == 0) ? 0.06f : -0.05f;
            PushCylinder(trunkVerts, trunkInds, glm::vec3(x, ground + height * 0.5f, z), 0.12f + static_cast<float>(i % 2) * 0.03f, height, 8, true, true);
            PushCone(branchVerts, branchInds, glm::vec3(x + sway, ground + height + 1.6f, z), 1.8f, 3.2f, 7);
            PushCone(branchVerts, branchInds, glm::vec3(x - sway * 0.5f, ground + height + 3.8f, z), 1.1f, 2.2f, 7);
        }
    }

    UploadBatch(trunkVerts, trunkInds, glm::vec3(0.86f, 0.84f, 0.76f), 0.84f);
    UploadBatch(branchVerts, branchInds, glm::vec3(0.20f, 0.24f, 0.18f), 0.92f);
}

void WorldBuilder::BuildBigRedMarker() {
    if (!terrain) {
        return;
    }

    std::vector<WorldSceneVertex> verts;
    std::vector<unsigned int> inds;

    const float ground = GetHeightAt(0.0f, 14.0f);
    PushBox(verts, inds, glm::vec3(0.0f, ground + 1.9f, 14.0f), glm::vec3(0.45f, 3.8f, 0.45f));
    PushBox(verts, inds, glm::vec3(0.0f, ground + 3.2f, 14.0f), glm::vec3(1.6f, 0.22f, 0.22f));

    UploadBatch(verts, inds, glm::vec3(0.63f, 0.12f, 0.10f), 0.74f);
}

void WorldBuilder::BuildAtmosphericProps() {
    if (!terrain) {
        return;
    }

    std::vector<WorldSceneVertex> poleVerts;
    std::vector<unsigned int> poleInds;
    std::vector<WorldSceneVertex> vehicleVerts;
    std::vector<unsigned int> vehicleInds;
    std::vector<WorldSceneVertex> signVerts;
    std::vector<unsigned int> signInds;
    std::vector<WorldSceneVertex> propVerts;
    std::vector<unsigned int> propInds;

    for (int i = -5; i <= 5; ++i) {
        const float z = static_cast<float>(i) * 44.0f;
        const float x = -11.0f;
        const float ground = GetHeightAt(x, z);
        PushCylinder(poleVerts, poleInds, glm::vec3(x, ground + 4.2f, z), 0.14f, 8.4f, 8, true, true);
        PushThinBeam(poleVerts, poleInds, glm::vec3(x - 1.0f, ground + 7.9f, z), glm::vec3(x + 1.0f, ground + 7.9f, z), 0.09f);
        if (i < 5) {
            const float nextZ = static_cast<float>(i + 1) * 44.0f;
            PushThinBeam(poleVerts, poleInds, glm::vec3(x + 0.9f, ground + 7.95f, z), glm::vec3(x + 0.9f, GetHeightAt(x, nextZ) + 7.95f, nextZ), 0.035f);
        }
    }

    const glm::vec3 wagonCenter(-8.0f, GetHeightAt(-8.0f, 36.0f) + 0.85f, 36.0f);
    const glm::vec3 pickupCenter(18.0f, GetHeightAt(18.0f, -42.0f) + 0.9f, -42.0f);
    const glm::vec3 rvCenter(34.0f, GetHeightAt(34.0f, 24.0f) + 1.25f, 24.0f);
    PushBox(vehicleVerts, vehicleInds, wagonCenter, glm::vec3(4.6f, 1.4f, 2.0f));
    PushBox(vehicleVerts, vehicleInds, wagonCenter + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(3.1f, 0.85f, 1.8f));
    PushBox(vehicleVerts, vehicleInds, pickupCenter, glm::vec3(4.9f, 1.5f, 2.1f));
    PushBox(vehicleVerts, vehicleInds, pickupCenter + glm::vec3(0.8f, 0.4f, 0.0f), glm::vec3(1.8f, 0.9f, 2.0f));
    PushBox(vehicleVerts, vehicleInds, rvCenter, glm::vec3(7.0f, 2.4f, 2.6f));
    PushBox(vehicleVerts, vehicleInds, rvCenter + glm::vec3(0.0f, 1.4f, 0.0f), glm::vec3(4.4f, 0.8f, 2.5f));

    const glm::vec3 busCenter(-106.0f, GetHeightAt(-106.0f, 12.0f) + 1.35f, 12.0f);
    PushBox(vehicleVerts, vehicleInds, busCenter, glm::vec3(11.2f, 2.9f, 3.0f));
    PushBox(vehicleVerts, vehicleInds, busCenter + glm::vec3(0.0f, 0.7f, 0.0f), glm::vec3(9.8f, 1.7f, 2.7f));
    for (int i = -3; i <= 3; ++i) {
        const float x = busCenter.x + static_cast<float>(i) * 1.2f;
        PushBox(vehicleVerts, vehicleInds, glm::vec3(x, busCenter.y + 0.25f, busCenter.z + 1.45f), glm::vec3(0.72f, 0.82f, 0.08f));
    }
    PushCylinder(vehicleVerts, vehicleInds, busCenter + glm::vec3(-3.7f, -0.72f, 1.25f), 0.48f, 0.72f, 8, true, true);
    PushCylinder(vehicleVerts, vehicleInds, busCenter + glm::vec3( 3.7f, -0.72f, 1.25f), 0.48f, 0.72f, 8, true, true);
    PushCylinder(vehicleVerts, vehicleInds, busCenter + glm::vec3(-3.7f, -0.72f,-1.25f), 0.48f, 0.72f, 8, true, true);
    PushCylinder(vehicleVerts, vehicleInds, busCenter + glm::vec3( 3.7f, -0.72f,-1.25f), 0.48f, 0.72f, 8, true, true);

    const float stopGround = GetHeightAt(26.0f, 0.0f);
    PushCylinder(signVerts, signInds, glm::vec3(26.0f, stopGround + 1.5f, 0.0f), 0.06f, 3.0f, 8, true, true);
    PushBox(signVerts, signInds, glm::vec3(26.0f, stopGround + 2.8f, 0.0f), glm::vec3(0.85f, 0.85f, 0.08f));
    PushCylinder(signVerts, signInds, glm::vec3(-26.0f, GetHeightAt(-26.0f, -60.0f) + 1.5f, -60.0f), 0.06f, 3.0f, 8, true, true);
    PushBox(signVerts, signInds, glm::vec3(-26.0f, GetHeightAt(-26.0f, -60.0f) + 2.8f, -60.0f), glm::vec3(0.85f, 0.85f, 0.08f));

    const float boothGround = GetHeightAt(14.0f, 44.0f);
    PushBox(propVerts, propInds, glm::vec3(14.0f, boothGround + 1.1f, 44.0f), glm::vec3(0.95f, 2.2f, 0.95f));
    PushBox(propVerts, propInds, glm::vec3(14.0f, boothGround + 0.8f, 44.35f), glm::vec3(0.30f, 0.55f, 0.08f));
    const std::array<glm::vec2, 6> mailboxes = {{
        {-116.0f, -79.0f}, {-95.0f, -55.0f}, {84.0f, 72.0f},
        {108.0f, 106.0f}, {42.0f, -86.0f}, {-16.0f, 120.0f}
    }};
    for (const glm::vec2& mailbox : mailboxes) {
        const float ground = GetHeightAt(mailbox.x, mailbox.y);
        PushCylinder(propVerts, propInds, glm::vec3(mailbox.x, ground + 0.55f, mailbox.y), 0.04f, 1.1f, 8, true, true);
        PushBox(propVerts, propInds, glm::vec3(mailbox.x, ground + 1.05f, mailbox.y + 0.1f), glm::vec3(0.34f, 0.24f, 0.32f));
    }
    PushBox(propVerts, propInds, glm::vec3(-42.0f, GetHeightAt(-42.0f, -14.0f) + 0.42f, -14.0f), glm::vec3(1.2f, 0.84f, 1.2f));
    PushBox(propVerts, propInds, glm::vec3(-40.0f, GetHeightAt(-40.0f, -14.0f) + 0.32f, -11.8f), glm::vec3(0.9f, 0.64f, 0.9f));

    const glm::vec3 beaconBase(-154.0f, GetHeightAt(-154.0f, 132.0f), 132.0f);
    PushCylinder(propVerts, propInds, beaconBase + glm::vec3(0.0f, 12.0f, 0.0f), 1.15f, 24.0f, 10, true, true);
    PushCylinder(propVerts, propInds, beaconBase + glm::vec3(0.0f, 24.0f, 0.0f), 0.62f, 5.5f, 8, true, true);
    PushBox(propVerts, propInds, beaconBase + glm::vec3(0.0f, 27.1f, 0.0f), glm::vec3(2.9f, 1.2f, 2.9f));
    PushBox(propVerts, propInds, beaconBase + glm::vec3(0.0f, 28.3f, 0.0f), glm::vec3(1.2f, 0.4f, 1.2f));

    UploadBatch(poleVerts, poleInds, glm::vec3(0.35f, 0.29f, 0.21f), 0.87f);
    UploadBatch(vehicleVerts, vehicleInds, glm::vec3(0.28f, 0.24f, 0.20f), 0.72f, 0.08f);
    UploadBatch(signVerts, signInds, glm::vec3(0.65f, 0.12f, 0.10f), 0.62f, 0.04f);
    UploadBatch(propVerts, propInds, glm::vec3(0.44f, 0.44f, 0.42f), 0.76f, 0.06f);
}

void WorldBuilder::BuildTownFacades() {
    if (!terrain) {
        return;
    }

    const struct FacadeBlock {
        glm::vec2 footprint;
        glm::vec2 sizeXZ;
        float height;
        int windowsX;
        int windowsY;
        bool gabledRoof;
        glm::vec3 wallColor;
        glm::vec3 trimColor;
        glm::vec3 roofColor;
        glm::vec3 signColor;
        glm::vec3 awningColor;
    } blocks[] = {
        {{-18.0f, 84.0f}, {16.0f, 10.0f}, 7.0f, 4, 2, true, {0.82f, 0.78f, 0.70f}, {0.20f, 0.18f, 0.16f}, {0.23f, 0.22f, 0.20f}, {0.58f, 0.12f, 0.10f}, {0.64f, 0.24f, 0.18f}},
        {{  4.0f, 84.0f}, {13.0f, 9.0f}, 6.4f, 3, 2, false, {0.74f, 0.82f, 0.76f}, {0.18f, 0.18f, 0.18f}, {0.24f, 0.26f, 0.24f}, {0.18f, 0.18f, 0.18f}, {0.30f, 0.24f, 0.18f}},
        {{ 22.0f, 82.0f}, {14.0f, 9.0f}, 6.5f, 3, 2, true, {0.78f, 0.72f, 0.58f}, {0.20f, 0.18f, 0.16f}, {0.23f, 0.22f, 0.21f}, {0.62f, 0.15f, 0.10f}, {0.52f, 0.30f, 0.18f}},
        {{ 42.0f, 80.0f}, {15.0f, 10.0f}, 6.8f, 4, 2, true, {0.86f, 0.83f, 0.74f}, {0.18f, 0.18f, 0.18f}, {0.22f, 0.22f, 0.20f}, {0.20f, 0.18f, 0.16f}, {0.62f, 0.52f, 0.24f}},
        {{-34.0f, 72.0f}, {12.0f, 8.0f}, 5.8f, 2, 2, true, {0.76f, 0.70f, 0.54f}, {0.21f, 0.18f, 0.16f}, {0.24f, 0.22f, 0.20f}, {0.58f, 0.16f, 0.10f}, {0.40f, 0.28f, 0.18f}},
        {{-12.0f, 98.0f}, {14.0f, 9.0f}, 5.6f, 3, 2, false, {0.84f, 0.78f, 0.52f}, {0.18f, 0.18f, 0.16f}, {0.24f, 0.22f, 0.20f}, {0.22f, 0.18f, 0.16f}, {0.60f, 0.52f, 0.20f}},
        {{-58.0f, 48.0f}, {15.0f, 9.0f}, 6.0f, 3, 2, true, {0.86f, 0.86f, 0.78f}, {0.16f, 0.16f, 0.16f}, {0.20f, 0.22f, 0.20f}, {0.18f, 0.18f, 0.18f}, {0.32f, 0.24f, 0.18f}},
        {{104.0f, 54.0f}, {18.0f, 11.0f}, 9.4f, 4, 3, true, {0.76f, 0.70f, 0.56f}, {0.20f, 0.18f, 0.16f}, {0.22f, 0.22f, 0.20f}, {0.62f, 0.12f, 0.10f}, {0.66f, 0.54f, 0.26f}},
    };

    auto buildPicketFence = [&](std::vector<WorldSceneVertex>& verts, std::vector<unsigned int>& inds, const glm::vec3& center, float width, float depth) {
        const float halfW = width * 0.5f;
        const float halfD = depth * 0.5f;
        const glm::vec3 northA = center + glm::vec3(-halfW, 0.0f, -halfD);
        const glm::vec3 northB = center + glm::vec3( halfW, 0.0f, -halfD);
        const glm::vec3 southA = center + glm::vec3(-halfW, 0.0f,  halfD);
        const glm::vec3 southB = center + glm::vec3( halfW, 0.0f,  halfD);

        for (int i = -3; i <= 3; ++i) {
            const float t = (static_cast<float>(i) + 3.0f) / 6.0f;
            PushCylinder(verts, inds, glm::mix(northA, northB, t) + glm::vec3(0.0f, 0.68f, 0.0f), 0.04f, 1.36f, 6, true, true);
            PushCylinder(verts, inds, glm::mix(southA, southB, t) + glm::vec3(0.0f, 0.68f, 0.0f), 0.04f, 1.36f, 6, true, true);
        }

        PushThinBeam(verts, inds, northA + glm::vec3(0.0f, 1.00f, 0.0f), northB + glm::vec3(0.0f, 1.00f, 0.0f), 0.03f);
        PushThinBeam(verts, inds, southA + glm::vec3(0.0f, 1.00f, 0.0f), southB + glm::vec3(0.0f, 1.00f, 0.0f), 0.03f);
    };

    for (const FacadeBlock& block : blocks) {
        std::vector<WorldSceneVertex> wallVerts;
        std::vector<unsigned int> wallInds;
        std::vector<WorldSceneVertex> trimVerts;
        std::vector<unsigned int> trimInds;
        std::vector<WorldSceneVertex> signVerts;
        std::vector<unsigned int> signInds;
        std::vector<WorldSceneVertex> streetVerts;
        std::vector<unsigned int> streetInds;

        const float ground = GetHeightAt(block.footprint.x, block.footprint.y);
        const glm::vec3 center(block.footprint.x, ground + block.height * 0.5f + 0.2f, block.footprint.y);
        const glm::vec3 bodySize(block.sizeXZ.x, block.height, block.sizeXZ.y);
        PushBox(wallVerts, wallInds, center, bodySize);

        const float front = center.z + bodySize.z * 0.5f + 0.08f;
        const float back = center.z - bodySize.z * 0.5f - 0.06f;
        const float left = center.x - bodySize.x * 0.5f + 0.8f;
        const float right = center.x + bodySize.x * 0.5f - 0.8f;
        const float bottom = center.y - bodySize.y * 0.5f + 1.0f;
        const float spacingX = (bodySize.x - 2.0f) / static_cast<float>(std::max(1, block.windowsX - 1));
        const float spacingY = (bodySize.y - 2.2f) / static_cast<float>(std::max(1, block.windowsY - 1));
        const float windowW = std::max(0.48f, bodySize.x / static_cast<float>(block.windowsX + 3) * 0.38f);
        const float windowH = std::max(0.62f, bodySize.y / static_cast<float>(block.windowsY + 3) * 0.28f);
        const float annexOffset = (block.footprint.x < 0.0f ? -1.0f : 1.0f) * bodySize.x * 0.26f;
        const float annexWidth = bodySize.x * 0.35f;
        const float annexDepth = bodySize.z * 0.68f;

        if (block.gabledRoof) {
            PushGabledRoof(trimVerts, trimInds, center + glm::vec3(0.0f, bodySize.y * 0.5f, 0.0f), bodySize.x + 0.55f, bodySize.z + 0.40f, 0.0f, bodySize.y * 0.38f, 0.25f);
        } else {
            PushBox(trimVerts, trimInds, center + glm::vec3(0.0f, bodySize.y * 0.5f + 0.24f, 0.0f), glm::vec3(bodySize.x + 0.4f, 0.36f, bodySize.z + 0.3f));
        }

        PushBox(wallVerts, wallInds, center + glm::vec3(annexOffset, -bodySize.y * 0.12f, -bodySize.z * 0.28f), glm::vec3(annexWidth, bodySize.y * 0.72f, annexDepth));
        PushBox(trimVerts, trimInds, center + glm::vec3(annexOffset, bodySize.y * 0.28f, -bodySize.z * 0.32f), glm::vec3(annexWidth + 0.26f, 0.28f, annexDepth + 0.18f));
        PushBox(streetVerts, streetInds, center + glm::vec3(annexOffset * 0.9f, -bodySize.y * 0.42f, bodySize.z * 0.34f), glm::vec3(1.1f, 0.18f, 0.9f));
        PushBox(streetVerts, streetInds, center + glm::vec3(annexOffset * 0.9f, -bodySize.y * 0.60f, bodySize.z * 0.62f), glm::vec3(0.82f, 0.16f, 0.72f));

        for (int y = 0; y < block.windowsY; ++y) {
            for (int x = 0; x < block.windowsX; ++x) {
                const float wx = left + static_cast<float>(x) * spacingX;
                const float wy = bottom + static_cast<float>(y) * spacingY;
                const glm::vec3 frameCenter(wx, wy, front);
                const glm::vec3 paneCenter(wx, wy, front + 0.05f);
                PushBox(trimVerts, trimInds, frameCenter, glm::vec3(windowW + 0.16f, windowH + 0.16f, 0.07f));
                PushBox(streetVerts, streetInds, paneCenter, glm::vec3(windowW * 0.70f, windowH * 0.70f, 0.03f));
                if (y == 0) {
                    PushBox(trimVerts, trimInds, frameCenter + glm::vec3(0.0f, -0.52f, 0.0f), glm::vec3(windowW + 0.22f, 0.10f, 0.08f));
                }
            }
        }

        const glm::vec3 doorCenter(center.x - bodySize.x * 0.18f, center.y - bodySize.y * 0.40f + 0.95f, front);
        PushBox(trimVerts, trimInds, doorCenter, glm::vec3(bodySize.x * 0.22f, bodySize.y * 0.42f, 0.10f));
        PushBox(signVerts, signInds, glm::vec3(center.x, center.y + bodySize.y * 0.46f, front), glm::vec3(bodySize.x * 0.68f, 0.22f, 0.08f));
        PushBox(signVerts, signInds, glm::vec3(center.x, center.y + bodySize.y * 0.58f, front + 0.02f), glm::vec3(bodySize.x * 0.38f, 0.18f, 0.06f));

        const float awningY = center.y - bodySize.y * 0.10f;
        PushBox(trimVerts, trimInds, glm::vec3(center.x, awningY, front + 0.02f), glm::vec3(bodySize.x * 0.70f, 0.20f, 0.18f));
        PushBox(streetVerts, streetInds, glm::vec3(center.x, awningY - 0.05f, front + 0.10f), glm::vec3(bodySize.x * 0.76f, 0.08f, 0.24f));

        PushBox(streetVerts, streetInds, glm::vec3(center.x - bodySize.x * 0.31f, center.y - bodySize.y * 0.45f + 0.25f, front + 0.20f), glm::vec3(0.22f, 0.50f, 0.22f));
        PushBox(streetVerts, streetInds, glm::vec3(center.x + bodySize.x * 0.30f, center.y - bodySize.y * 0.40f + 0.28f, front + 0.14f), glm::vec3(0.26f, 0.24f, 0.24f));
        PushCylinder(streetVerts, streetInds, glm::vec3(center.x + bodySize.x * 0.37f, center.y - bodySize.y * 0.42f + 0.55f, front + 0.22f), 0.08f, 1.0f, 8, true, true);

        PushBox(streetVerts, streetInds, glm::vec3(center.x, ground + 0.10f, center.z + bodySize.z * 0.52f), glm::vec3(bodySize.x * 0.95f, 0.08f, 0.34f));
        PushBox(streetVerts, streetInds, glm::vec3(center.x, ground + 0.02f, center.z + bodySize.z * 0.72f), glm::vec3(bodySize.x * 0.75f, 0.06f, 0.16f));

        if (center.x < 0.0f) {
            PushBox(streetVerts, streetInds, glm::vec3(center.x - bodySize.x * 0.58f, center.y - bodySize.y * 0.05f, back), glm::vec3(0.40f, bodySize.y * 0.62f, bodySize.z * 0.68f));
            PushBox(streetVerts, streetInds, glm::vec3(center.x - bodySize.x * 0.84f, center.y - bodySize.y * 0.22f, back - 0.12f), glm::vec3(0.92f, 0.66f, bodySize.z * 0.72f));
        } else {
            PushBox(streetVerts, streetInds, glm::vec3(center.x + bodySize.x * 0.58f, center.y - bodySize.y * 0.05f, back), glm::vec3(0.40f, bodySize.y * 0.62f, bodySize.z * 0.68f));
            PushBox(streetVerts, streetInds, glm::vec3(center.x + bodySize.x * 0.84f, center.y - bodySize.y * 0.22f, back - 0.12f), glm::vec3(0.92f, 0.66f, bodySize.z * 0.72f));
        }

        if (center.z > 70.0f) {
            PushThinBeam(streetVerts, streetInds, glm::vec3(left - 1.0f, center.y + bodySize.y * 0.52f, front), glm::vec3(right + 1.0f, center.y + bodySize.y * 0.52f, front), 0.06f);
            PushThinBeam(streetVerts, streetInds, glm::vec3(left - 1.0f, center.y + bodySize.y * 0.62f, front), glm::vec3(right + 1.0f, center.y + bodySize.y * 0.62f, front), 0.05f);
        }

        const glm::vec3 signBase(center.x, center.y + bodySize.y * 0.64f, front + 0.10f);
        PushBox(signVerts, signInds, signBase, glm::vec3(bodySize.x * 0.78f, 0.18f, 0.07f));
        PushBox(signVerts, signInds, signBase + glm::vec3(0.0f, 0.18f, 0.0f), glm::vec3(bodySize.x * 0.34f, 0.12f, 0.05f));

        if (block.windowsX >= 4) {
            PushBox(signVerts, signInds, glm::vec3(center.x, center.y + bodySize.y * 0.70f, front + 0.12f), glm::vec3(bodySize.x * 0.50f, 0.10f, 0.04f));
        }

        PushCylinder(streetVerts, streetInds, glm::vec3(center.x + bodySize.x * 0.33f, center.y - bodySize.y * 0.42f, front + 0.18f), 0.10f, 1.2f, 8, true, true);
        PushCylinder(streetVerts, streetInds, glm::vec3(center.x - bodySize.x * 0.35f, center.y - bodySize.y * 0.42f, front + 0.18f), 0.10f, 1.2f, 8, true, true);
        PushBox(streetVerts, streetInds, glm::vec3(center.x, ground + 0.08f, center.z + bodySize.z * 0.58f), glm::vec3(bodySize.x * 0.74f, 0.06f, 0.18f));
        PushBox(streetVerts, streetInds, glm::vec3(center.x, ground + 0.16f, center.z + bodySize.z * 0.78f), glm::vec3(bodySize.x * 0.52f, 0.06f, 0.16f));

        buildPicketFence(streetVerts, streetInds, glm::vec3(center.x + (block.footprint.x < 0.0f ? -bodySize.x * 0.72f : bodySize.x * 0.72f), ground + 0.02f, center.z + bodySize.z * 0.56f), bodySize.x * 0.86f, bodySize.z * 0.32f);

        PushBox(signVerts, signInds, glm::vec3(center.x, center.y + bodySize.y * 0.82f, front + 0.14f), glm::vec3(bodySize.x * 0.20f, 0.08f, 0.04f));
        PushBox(trimVerts, trimInds, glm::vec3(center.x, center.y + bodySize.y * 0.36f, front + 0.04f), glm::vec3(bodySize.x * 0.94f, 0.08f, 0.06f));

        UploadBatch(wallVerts, wallInds, block.wallColor, 0.84f, GetWallTexture());
        UploadBatch(trimVerts, trimInds, block.trimColor, 0.64f, GetTrimTexture());
        UploadBatch(signVerts, signInds, block.signColor, 0.52f, GetSignTexture(), 0.06f, block.signColor * 0.10f);
        UploadBatch(streetVerts, streetInds, block.awningColor, 0.80f, GetRoofTexture(), 0.02f);
    }

    const std::array<glm::vec3, 6> lampPosts = {{
        {-20.0f, GetHeightAt(-20.0f, 82.0f), 82.0f},
        {  4.0f, GetHeightAt(  4.0f, 82.0f), 82.0f},
        { 24.0f, GetHeightAt( 24.0f, 80.0f), 80.0f},
        { 46.0f, GetHeightAt( 46.0f, 76.0f), 76.0f},
        {-50.0f, GetHeightAt(-50.0f, 50.0f), 50.0f},
        { 82.0f, GetHeightAt( 82.0f, 54.0f), 54.0f},
    }};

    std::vector<WorldSceneVertex> streetVerts;
    std::vector<unsigned int> streetInds;
    for (const glm::vec3& lamp : lampPosts) {
        PushCylinder(streetVerts, streetInds, lamp + glm::vec3(0.0f, 4.2f, 0.0f), 0.08f, 8.4f, 8, true, true);
        PushThinBeam(streetVerts, streetInds, lamp + glm::vec3(-0.8f, 7.8f, 0.0f), lamp + glm::vec3(0.8f, 7.8f, 0.0f), 0.06f);
        PushBox(streetVerts, streetInds, lamp + glm::vec3(0.9f, 8.2f, 0.0f), glm::vec3(0.28f, 0.24f, 0.22f));
        PushBox(streetVerts, streetInds, lamp + glm::vec3(0.9f, 8.55f, 0.0f), glm::vec3(0.16f, 0.16f, 0.18f));
    }

    const std::array<glm::vec3, 4> crossings = {{
        {-14.0f, GetHeightAt(-14.0f, 82.0f), 82.0f},
        { 10.0f, GetHeightAt( 10.0f, 82.0f), 82.0f},
        { 40.0f, GetHeightAt( 40.0f, 80.0f), 80.0f},
        { 72.0f, GetHeightAt( 72.0f, 74.0f), 74.0f},
    }};

    for (const glm::vec3& cross : crossings) {
        PushBox(streetVerts, streetInds, cross + glm::vec3(0.0f, 0.06f, 0.0f), glm::vec3(4.8f, 0.05f, 1.2f));
        PushBox(streetVerts, streetInds, cross + glm::vec3(0.0f, 0.08f, 1.6f), glm::vec3(4.8f, 0.05f, 0.8f));
    }

    const glm::vec3 busStop(-106.0f, GetHeightAt(-106.0f, 12.0f), 12.0f);
    PushBox(streetVerts, streetInds, busStop + glm::vec3(-1.5f, 0.06f, 0.0f), glm::vec3(8.0f, 0.08f, 3.8f));
    PushBox(streetVerts, streetInds, busStop + glm::vec3(4.2f, 0.7f, 1.55f), glm::vec3(0.18f, 1.4f, 0.18f));
    PushBox(streetVerts, streetInds, busStop + glm::vec3(4.2f, 1.8f, 1.55f), glm::vec3(1.8f, 0.36f, 0.1f));

    UploadBatch(streetVerts, streetInds, glm::vec3(0.30f, 0.30f, 0.29f), 0.82f, GetStreetTexture(), 0.02f);
}

void WorldBuilder::BuildBottleTreeShrine() {
    if (!terrain) {
        return;
    }

    std::vector<WorldSceneVertex> trunkVerts;
    std::vector<unsigned int> trunkInds;
    std::vector<WorldSceneVertex> bottleVerts;
    std::vector<unsigned int> bottleInds;

    const glm::vec3 treeCenter(-58.0f, GetHeightAt(-58.0f, 18.0f), 18.0f);
    PushCylinder(trunkVerts, trunkInds, treeCenter + glm::vec3(0.0f, 3.4f, 0.0f), 0.38f, 6.8f, 10, true, true);
    PushCylinder(trunkVerts, trunkInds, treeCenter + glm::vec3(-0.8f, 6.8f, 0.3f), 0.20f, 3.6f, 8, true, true);
    PushCylinder(trunkVerts, trunkInds, treeCenter + glm::vec3(0.6f, 7.2f, -0.4f), 0.16f, 3.0f, 8, true, true);

    for (int i = 0; i < 16; ++i) {
        const float angle = glm::two_pi<float>() * static_cast<float>(i) / 16.0f;
        const float reach = 1.0f + static_cast<float>(i % 4) * 0.8f;
        const glm::vec3 branchStart = treeCenter + glm::vec3(std::cos(angle) * 0.6f, 5.6f + static_cast<float>(i % 3) * 0.8f, std::sin(angle) * 0.6f);
        const glm::vec3 branchEnd = branchStart + glm::vec3(std::cos(angle) * reach, 0.8f, std::sin(angle) * reach);
        PushThinBeam(trunkVerts, trunkInds, branchStart, branchEnd, 0.05f);

        const glm::vec3 bottleCenter = branchEnd + glm::vec3(0.0f, -0.18f, 0.0f);
        PushBox(bottleVerts, bottleInds, bottleCenter, glm::vec3(0.12f, 0.36f, 0.12f));
        PushBox(bottleVerts, bottleInds, bottleCenter + glm::vec3(0.0f, 0.15f, 0.0f), glm::vec3(0.08f, 0.12f, 0.08f));
    }

    for (int i = 0; i < 4; ++i) {
        const float angle = glm::two_pi<float>() * static_cast<float>(i) / 4.0f;
        const glm::vec3 postPos = treeCenter + glm::vec3(std::cos(angle) * 3.2f, 0.55f, std::sin(angle) * 2.8f);
        PushCylinder(trunkVerts, trunkInds, postPos, 0.06f, 1.1f, 6, true, true);
        PushBox(bottleVerts, bottleInds, postPos + glm::vec3(0.0f, 0.6f, 0.0f), glm::vec3(0.18f, 0.18f, 0.18f));
    }

    UploadBatch(trunkVerts, trunkInds, glm::vec3(0.24f, 0.16f, 0.11f), 0.94f, GetTrimTexture());
    UploadBatch(bottleVerts, bottleInds, glm::vec3(0.18f, 0.38f, 0.42f), 0.26f, GetSignTexture(), 0.02f);
}

void WorldBuilder::BuildFencedLots() {
    if (!terrain) {
        return;
    }

    std::vector<WorldSceneVertex> fenceVerts;
    std::vector<unsigned int> fenceInds;
    std::vector<WorldSceneVertex> graveVerts;
    std::vector<unsigned int> graveInds;

    auto addFenceLoop = [&](glm::vec3 center, glm::vec2 size, float height, float spacing) {
        const float halfX = size.x * 0.5f;
        const float halfZ = size.y * 0.5f;
        const glm::vec3 corners[4] = {
            center + glm::vec3(-halfX, 0.0f, -halfZ),
            center + glm::vec3( halfX, 0.0f, -halfZ),
            center + glm::vec3( halfX, 0.0f,  halfZ),
            center + glm::vec3(-halfX, 0.0f,  halfZ),
        };

        for (int edge = 0; edge < 4; ++edge) {
            const glm::vec3 start = corners[edge];
            const glm::vec3 end = corners[(edge + 1) % 4];
            const float length = glm::length(end - start);
            const int posts = std::max(2, static_cast<int>(std::floor(length / spacing)));
            for (int i = 0; i <= posts; ++i) {
                const float t = static_cast<float>(i) / static_cast<float>(posts);
                const glm::vec3 postBase = glm::mix(start, end, t);
                PushCylinder(fenceVerts, fenceInds, postBase + glm::vec3(0.0f, height * 0.5f, 0.0f), 0.05f, height, 8, true, true);
            }

            PushThinBeam(fenceVerts, fenceInds, start + glm::vec3(0.0f, height * 0.70f, 0.0f), end + glm::vec3(0.0f, height * 0.70f, 0.0f), 0.04f);
            PushThinBeam(fenceVerts, fenceInds, start + glm::vec3(0.0f, height * 0.38f, 0.0f), end + glm::vec3(0.0f, height * 0.38f, 0.0f), 0.04f);
        }
    };

    const float cemeteryGround = GetHeightAt(-132.0f, 24.0f);
    addFenceLoop(glm::vec3(-132.0f, cemeteryGround, 24.0f), glm::vec2(18.0f, 12.0f), 1.3f, 1.8f);
    for (int i = 0; i < 6; ++i) {
        const float x = -138.0f + static_cast<float>(i % 3) * 4.8f;
        const float z = 20.0f + static_cast<float>(i / 3) * 3.8f;
        const float ground = GetHeightAt(x, z);
        PushBox(graveVerts, graveInds, glm::vec3(x, ground + 0.45f, z), glm::vec3(0.55f, 0.9f, 0.18f));
    }
    PushCylinder(graveVerts, graveInds, glm::vec3(-125.0f, cemeteryGround + 3.0f, 27.0f), 0.22f, 6.0f, 10, true, true);
    PushCone(graveVerts, graveInds, glm::vec3(-125.0f, cemeteryGround + 7.2f, 27.0f), 2.8f, 4.2f, 8);

    const float barricadeGround = GetHeightAt(0.0f, 186.0f);
    PushBox(graveVerts, graveInds, glm::vec3(-4.0f, barricadeGround + 0.9f, 186.0f), glm::vec3(5.5f, 1.8f, 2.1f));
    PushBox(graveVerts, graveInds, glm::vec3(4.6f, barricadeGround + 0.8f, 186.5f), glm::vec3(4.6f, 1.6f, 2.0f));
    for (int i = -4; i <= 4; ++i) {
        PushThinBeam(fenceVerts, fenceInds, glm::vec3(static_cast<float>(i) * 1.2f, barricadeGround + 0.2f, 183.8f), glm::vec3(static_cast<float>(i) * 1.2f, barricadeGround + 2.8f, 188.6f), 0.08f);
    }

    UploadBatch(fenceVerts, fenceInds, glm::vec3(0.28f, 0.24f, 0.20f), 0.88f);
    UploadBatch(graveVerts, graveInds, glm::vec3(0.52f, 0.50f, 0.48f), 0.84f);
}

void WorldBuilder::BuildTunnelEntrances() {
    if (!terrain) {
        return;
    }

    std::vector<WorldSceneVertex> stoneVerts;
    std::vector<unsigned int> stoneInds;
    std::vector<WorldSceneVertex> doorVerts;
    std::vector<unsigned int> doorInds;

    const std::array<glm::vec2, 4> tunnelPositions = {{
        {-168.0f, 116.0f},
        {-176.0f, -138.0f},
        {164.0f, -148.0f},
        {170.0f, 132.0f},
    }};

    for (const glm::vec2& tunnel : tunnelPositions) {
        const float ground = GetHeightAt(tunnel.x, tunnel.y);
        const glm::vec3 center(tunnel.x, ground + 1.2f, tunnel.y);
        PushBox(stoneVerts, stoneInds, center, glm::vec3(3.4f, 2.6f, 2.2f));
        PushBox(doorVerts, doorInds, center + glm::vec3(0.0f, -0.1f, 1.12f), glm::vec3(1.8f, 2.2f, 0.16f));
        PushBox(doorVerts, doorInds, center + glm::vec3(0.0f, -0.6f, 0.0f), glm::vec3(1.6f, 1.0f, 1.2f));
    }

    UploadBatch(stoneVerts, stoneInds, glm::vec3(0.36f, 0.35f, 0.34f), 0.94f);
    UploadBatch(doorVerts, doorInds, glm::vec3(0.15f, 0.10f, 0.08f), 0.82f);
}

void WorldBuilder::BuildSheriffStation() {
    if (!terrain) {
        return;
    }

    std::vector<WorldSceneVertex> wallVerts;
    std::vector<unsigned int> wallInds;
    std::vector<WorldSceneVertex> roofVerts;
    std::vector<unsigned int> roofInds;
    std::vector<WorldSceneVertex> signVerts;
    std::vector<unsigned int> signInds;

    const float ground = GetHeightAt(22.0f, -18.0f);
    const glm::vec3 center(22.0f, ground + 2.3f, -18.0f);
    PushBox(wallVerts, wallInds, center, glm::vec3(9.2f, 4.6f, 6.4f));
    PushGabledRoof(roofVerts, roofInds, center + glm::vec3(0.0f, 2.3f, 0.0f), 9.6f, 6.8f, 0.0f, 1.8f, 0.3f);
    PushBox(signVerts, signInds, center + glm::vec3(0.0f, 1.6f, 3.3f), glm::vec3(2.0f, 0.5f, 0.1f));

    UploadBatch(wallVerts, wallInds, glm::vec3(0.50f, 0.52f, 0.50f), 0.84f);
    UploadBatch(roofVerts, roofInds, glm::vec3(0.22f, 0.28f, 0.20f), 0.88f);
    UploadBatch(signVerts, signInds, glm::vec3(0.62f, 0.54f, 0.22f), 0.66f, 0.12f);
}

void WorldBuilder::BuildWaterTower() {
    if (!terrain) {
        return;
    }

    std::vector<WorldSceneVertex> metalVerts;
    std::vector<unsigned int> metalInds;
    std::vector<WorldSceneVertex> woodVerts;
    std::vector<unsigned int> woodInds;

    const float ground = GetHeightAt(54.0f, 38.0f);
    const glm::vec3 tankCenter(54.0f, ground + 14.0f, 38.0f);

    PushCylinder(woodVerts, woodInds, tankCenter, 4.0f, 4.0f, 14, true, true);
    const std::array<glm::vec2, 4> legOffsets = {{
        {-2.8f, -2.8f},
        { 2.8f, -2.8f},
        {-2.8f,  2.8f},
        { 2.8f,  2.8f},
    }};
    for (const glm::vec2& off : legOffsets) {
        PushCylinder(metalVerts, metalInds, glm::vec3(54.0f + off.x, ground + 6.0f, 38.0f + off.y), 0.18f, 12.0f, 8, true, true);
    }

    for (int i = -2; i <= 2; ++i) {
        PushThinBeam(metalVerts, metalInds, glm::vec3(51.2f, ground + 4.0f + static_cast<float>(i), 35.2f), glm::vec3(56.8f, ground + 4.0f + static_cast<float>(i), 40.8f), 0.08f);
        PushThinBeam(metalVerts, metalInds, glm::vec3(56.8f, ground + 4.0f + static_cast<float>(i), 35.2f), glm::vec3(51.2f, ground + 4.0f + static_cast<float>(i), 40.8f), 0.08f);
    }

    UploadBatch(woodVerts, woodInds, glm::vec3(0.46f, 0.40f, 0.31f), 0.82f);
    UploadBatch(metalVerts, metalInds, glm::vec3(0.32f, 0.34f, 0.36f), 0.56f, 0.22f);
}

void WorldBuilder::BuildStoneCircles() {
    if (!terrain) {
        return;
    }

    std::vector<WorldSceneVertex> stoneVerts;
    std::vector<unsigned int> stoneInds;

    const std::array<glm::vec3, 3> circleCenters = {{
        {-34.0f, GetHeightAt(-34.0f, -22.0f), -22.0f},
        {22.0f, GetHeightAt(22.0f, -18.0f), -18.0f},
        {-8.0f, GetHeightAt(-8.0f, 84.0f), 84.0f},
    }};

    for (const glm::vec3& center : circleCenters) {
        for (int i = 0; i < 8; ++i) {
            const float angle = glm::two_pi<float>() * static_cast<float>(i) / 8.0f;
            const float radius = 2.6f + static_cast<float>(i % 2) * 0.7f;
            const glm::vec3 stonePos = center + glm::vec3(std::cos(angle) * radius, 0.38f + static_cast<float>(i % 3) * 0.08f, std::sin(angle) * radius);
            PushBox(stoneVerts, stoneInds, stonePos, glm::vec3(0.18f, 0.72f + static_cast<float>(i % 2) * 0.14f, 0.16f));
        }
        PushBox(stoneVerts, stoneInds, center + glm::vec3(0.0f, 0.58f, 0.0f), glm::vec3(0.42f, 1.18f, 0.26f));
    }

    UploadBatch(stoneVerts, stoneInds, glm::vec3(0.52f, 0.50f, 0.46f), 0.86f, GetStreetTexture());
}

void WorldBuilder::BuildSky() {
}

void WorldBuilder::BuildFog() {
}

void WorldBuilder::ClearSceneBatches() {
    for (WorldSceneBatch& batch : sceneBatches) {
        if (batch.ebo) glDeleteBuffers(1, &batch.ebo);
        if (batch.vbo) glDeleteBuffers(1, &batch.vbo);
        if (batch.vao) glDeleteVertexArrays(1, &batch.vao);
        batch = WorldSceneBatch{};
    }
    sceneBatches.clear();
}

void WorldBuilder::UploadBatch(const std::vector<WorldSceneVertex>& verts,
                               const std::vector<unsigned int>& inds,
                               glm::vec3 albedo,
                               float roughness,
                               GLuint textureID,
                               float metalness,
                               glm::vec3 emission) {
    if (verts.empty() || inds.empty()) {
        return;
    }

    WorldSceneBatch batch;
    batch.albedo = albedo;
    batch.roughness = roughness;
    batch.metalness = metalness;
    batch.emission = emission;
    batch.textureID = textureID;
    batch.indexCount = static_cast<int>(inds.size());

    glGenVertexArrays(1, &batch.vao);
    glGenBuffers(1, &batch.vbo);
    glGenBuffers(1, &batch.ebo);

    glBindVertexArray(batch.vao);
    glBindBuffer(GL_ARRAY_BUFFER, batch.vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(verts.size() * sizeof(WorldSceneVertex)), verts.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batch.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(inds.size() * sizeof(unsigned int)), inds.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(WorldSceneVertex), reinterpret_cast<void*>(offsetof(WorldSceneVertex, pos)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(WorldSceneVertex), reinterpret_cast<void*>(offsetof(WorldSceneVertex, nrm)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(WorldSceneVertex), reinterpret_cast<void*>(offsetof(WorldSceneVertex, uv)));

    glBindVertexArray(0);
    sceneBatches.push_back(batch);
}
