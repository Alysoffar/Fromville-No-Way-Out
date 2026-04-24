#include "Terrain.h"

#include <algorithm>
#include <array>
#include <cmath>

#include <glm/gtc/noise.hpp>
#include <stb_image.h>

#include "renderer/Shader.h"

namespace {

float DistanceToSegment2D(const glm::vec2& point, const glm::vec2& a, const glm::vec2& b) {
    const glm::vec2 ab = b - a;
    const float denom = glm::dot(ab, ab);
    if (denom < 1e-5f) {
        return glm::length(point - a);
    }

    const float t = glm::clamp(glm::dot(point - a, ab) / denom, 0.0f, 1.0f);
    return glm::length(point - (a + ab * t));
}

GLuint LoadGrassTexture() {
    static GLuint texture = 0;
    if (texture != 0) {
        return texture;
    }

    const int width = 256;
    const int height = 256;
    std::vector<unsigned char> pixels(static_cast<size_t>(width * height * 4), 0);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const float u = static_cast<float>(x) / static_cast<float>(width - 1);
            const float v = static_cast<float>(y) / static_cast<float>(height - 1);
            const float blade = 0.5f + 0.5f * std::sin(u * 78.0f + v * 41.0f + static_cast<float>((x * 13 + y * 17) % 29));
            const float clump = 0.5f + 0.5f * std::sin(u * 14.0f + v * 9.0f + static_cast<float>((x * 7 + y * 5) % 11));
            const float dirt = glm::clamp(0.15f + 0.55f * std::sin(u * 5.0f + v * 6.0f), 0.0f, 1.0f);
            const float moisture = glm::clamp(0.5f + 0.5f * std::cos(u * 3.2f + v * 2.7f), 0.0f, 1.0f);
            const float patch = 0.5f + 0.5f * std::sin(u * 10.0f + v * 7.0f + static_cast<float>((x * 19 + y * 31) % 23));
            const float clover = 0.5f + 0.5f * std::sin(u * 24.0f + v * 19.0f + static_cast<float>((x * 11 + y * 13) % 17));

            glm::vec3 color(0.15f, 0.30f, 0.11f);
            color = glm::mix(color, glm::vec3(0.21f, 0.40f, 0.17f), blade * 0.72f);
            color = glm::mix(color, glm::vec3(0.12f, 0.24f, 0.09f), clump * 0.52f);
            color = glm::mix(color, glm::vec3(0.24f, 0.43f, 0.18f), patch * 0.34f);
            color += glm::vec3(0.03f, 0.05f, 0.02f) * moisture;
            color -= glm::vec3(0.05f, 0.03f, 0.02f) * dirt;
            color += glm::vec3(0.02f, 0.04f, 0.02f) * clover;
            color += glm::vec3(0.02f, 0.03f, 0.02f) * (0.5f + 0.5f * std::sin(u * 112.0f + v * 67.0f));

            const size_t index = static_cast<size_t>((y * width + x) * 4);
            pixels[index + 0] = static_cast<unsigned char>(glm::clamp(color.r, 0.0f, 1.0f) * 255.0f);
            pixels[index + 1] = static_cast<unsigned char>(glm::clamp(color.g, 0.0f, 1.0f) * 255.0f);
            pixels[index + 2] = static_cast<unsigned char>(glm::clamp(color.b, 0.0f, 1.0f) * 255.0f);
            pixels[index + 3] = 255;
        }
    }

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

} // namespace

Terrain::Terrain(int gridWidth, int gridDepth, float cellSize)
    : gridW(gridWidth), gridD(gridDepth), cell(cellSize) {}

Terrain::~Terrain() {
    if (ebo) glDeleteBuffers(1, &ebo);
    if (vbo) glDeleteBuffers(1, &vbo);
    if (vao) glDeleteVertexArrays(1, &vao);
    if (grassTexture) glDeleteTextures(1, &grassTexture);
}

void Terrain::Generate() {
    GenerateHeightMap();
    BuildMesh();
}

void Terrain::Draw(Shader& shader) const {
    if (!vao || indexCount <= 0) {
        return;
    }

    shader.SetMat4("model", glm::mat4(1.0f));
    shader.SetInt("baseTexture", 0);
    shader.SetBool("useTexture", true);
    shader.SetVec3("albedoColor", glm::vec3(0.30f, 0.38f, 0.24f));
    shader.SetFloat("roughness", 0.95f);
    shader.SetFloat("metalness", 0.0f);
    shader.SetVec3("emission", glm::vec3(0.0f));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, LoadGrassTexture());

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

float Terrain::GetHeightAt(float x, float z) const {
    if (heightMap.empty()) {
        return 0.0f;
    }

    const float gx = x / cell + static_cast<float>(gridW) * 0.5f;
    const float gz = z / cell + static_cast<float>(gridD) * 0.5f;

    const int x0 = std::clamp(static_cast<int>(std::floor(gx)), 0, gridW - 1);
    const int z0 = std::clamp(static_cast<int>(std::floor(gz)), 0, gridD - 1);
    const int x1 = std::min(x0 + 1, gridW - 1);
    const int z1 = std::min(z0 + 1, gridD - 1);

    const float tx = gx - static_cast<float>(x0);
    const float tz = gz - static_cast<float>(z0);

    const float h00 = heightMap[z0 * gridW + x0];
    const float h10 = heightMap[z0 * gridW + x1];
    const float h01 = heightMap[z1 * gridW + x0];
    const float h11 = heightMap[z1 * gridW + x1];

    const float hx0 = h00 + (h10 - h00) * tx;
    const float hx1 = h01 + (h11 - h01) * tx;
    return hx0 + (hx1 - hx0) * tz;
}

void Terrain::GenerateHeightMap() {
    heightMap.assign(static_cast<size_t>(gridW * gridD), 0.0f);

    auto ridgeAt = [](float wx, float wz, const glm::vec2& center, const glm::vec2& scale, float height, float sharpness) {
        const glm::vec2 delta((wx - center.x) / scale.x, (wz - center.y) / scale.y);
        const float dist = glm::length(delta);
        const float mask = glm::clamp(1.0f - dist, 0.0f, 1.0f);
        return height * std::pow(mask, sharpness);
    };

    struct RoadProfile {
        glm::vec2 start;
        glm::vec2 end;
        float width;
    };

    const std::array<RoadProfile, 20> roadProfiles = {{
        {{  0.0f, -320.0f}, {  0.0f, 320.0f}, 9.0f},
        {{-300.0f,   0.0f}, {300.0f,   0.0f}, 9.0f},
        {{-80.0f,  -60.0f}, { 80.0f, -60.0f}, 6.5f},
        {{-80.0f,   60.0f}, { 80.0f,  60.0f}, 6.5f},
        {{-80.0f,  -60.0f}, {-80.0f,  60.0f}, 6.5f},
        {{ 80.0f,  -60.0f}, { 80.0f,  60.0f}, 6.5f},
        {{-82.0f,  -24.0f}, {-12.0f,  -8.0f}, 5.0f},
        {{  4.0f,  -18.0f}, { 28.0f, -18.0f}, 4.6f},
        {{ 22.0f,  -24.0f}, { 42.0f, -68.0f}, 4.4f},
        {{ 28.0f,   28.0f}, { 74.0f,  74.0f}, 4.8f},
        {{ 36.0f,   22.0f}, { 58.0f,  38.0f}, 4.2f},
        {{-22.0f,   34.0f}, {-76.0f,  56.0f}, 4.8f},
        {{-18.0f,   40.0f}, {-18.0f, 128.0f}, 4.2f},
        {{ 62.0f,  -18.0f}, {114.0f, -20.0f}, 4.4f},
        {{-108.0f,  12.0f}, {-68.0f,  10.0f}, 5.6f},
        {{ -70.0f,  10.0f}, {-34.0f, -22.0f}, 4.6f},
        {{  -8.0f,  84.0f}, { 42.0f,  84.0f}, 5.0f},
        {{  42.0f,  84.0f}, { 78.0f,  74.0f}, 4.8f},
        {{  78.0f,  74.0f}, {104.0f,  54.0f}, 4.6f},
        {{ -58.0f,  48.0f}, {-82.0f,  54.0f}, 4.4f},
    }};

    for (int z = 0; z < gridD; ++z) {
        for (int x = 0; x < gridW; ++x) {
            const float wx = (static_cast<float>(x) - static_cast<float>(gridW) * 0.5f) * cell;
            const float wz = (static_cast<float>(z) - static_cast<float>(gridD) * 0.5f) * cell;

            float n = 0.0f;
            float amp = 1.0f;
            float freq = 0.0026f;
            for (int i = 0; i < 5; ++i) {
                n += glm::perlin(glm::vec2(wx * freq, wz * freq)) * amp;
                amp *= 0.5f;
                freq *= 2.0f;
            }

            const float broadNoise = glm::perlin(glm::vec2(wx * 0.0015f, wz * 0.0015f)) * 0.8f;
            const float detailNoise = glm::perlin(glm::vec2(wx * 0.010f, wz * 0.010f)) * 0.12f;
            float h = n * 0.95f + broadNoise * 0.55f + detailNoise;

            h += ridgeAt(wx, wz, glm::vec2(-146.0f, 112.0f), glm::vec2(84.0f, 62.0f), 4.4f, 2.0f);
            h += ridgeAt(wx, wz, glm::vec2(160.0f, 104.0f), glm::vec2(92.0f, 70.0f), 5.1f, 2.0f);
            h += ridgeAt(wx, wz, glm::vec2(-136.0f, -128.0f), glm::vec2(88.0f, 64.0f), 4.0f, 2.1f);
            h += ridgeAt(wx, wz, glm::vec2(148.0f, -146.0f), glm::vec2(96.0f, 72.0f), 4.6f, 2.0f);

            h += ridgeAt(wx, wz, glm::vec2(-210.0f, 166.0f), glm::vec2(140.0f, 116.0f), 2.8f, 2.4f);
            h += ridgeAt(wx, wz, glm::vec2(214.0f, 154.0f), glm::vec2(146.0f, 120.0f), 3.1f, 2.3f);
            h += ridgeAt(wx, wz, glm::vec2(-202.0f, -176.0f), glm::vec2(136.0f, 114.0f), 2.6f, 2.4f);
            h += ridgeAt(wx, wz, glm::vec2(208.0f, -168.0f), glm::vec2(148.0f, 118.0f), 2.9f, 2.3f);

            const float farRing = glm::smoothstep(180.0f, 270.0f, std::max(std::abs(wx), std::abs(wz)));
            h += farRing * 0.8f;

            const glm::vec2 pos(wx, wz);
            const float townExtent = std::max(std::abs(wx), std::abs(wz));
            const float townBlend = 1.0f - glm::smoothstep(70.0f, 220.0f, townExtent);
            h *= glm::mix(1.0f, 0.36f, townBlend);

            float roadBlend = 0.0f;
            for (const RoadProfile& road : roadProfiles) {
                const float dist = DistanceToSegment2D(pos, road.start, road.end);
                roadBlend = std::max(roadBlend, 1.0f - glm::smoothstep(road.width, road.width + 8.0f, dist));
            }
            h = glm::mix(h, 0.04f + broadNoise * 0.10f, roadBlend);

            struct SettlementPad {
                glm::vec2 center;
                float radius;
                float height;
                float roughness;
            };

            const std::array<SettlementPad, 8> settlementPads = {{
                {{   0.0f,   0.0f}, 42.0f, 0.02f, 0.04f},
                {{ -34.0f, -22.0f}, 26.0f, 0.08f, 0.05f},
                {{  -8.0f,  84.0f}, 32.0f, 0.06f, 0.04f},
                {{  42.0f, -68.0f}, 28.0f, 0.10f, 0.05f},
                {{  86.0f, -18.0f}, 30.0f, 0.12f, 0.05f},
                {{  78.0f,  74.0f}, 30.0f, 0.08f, 0.04f},
                {{ -58.0f,  48.0f}, 26.0f, 0.16f, 0.05f},
                {{ 104.0f,  54.0f}, 28.0f, 0.12f, 0.05f},
            }};

            for (const SettlementPad& pad : settlementPads) {
                const float dist = glm::length(pos - pad.center);
                const float mask = glm::clamp(1.0f - dist / pad.radius, 0.0f, 1.0f);
                const float padHeight = pad.height + broadNoise * pad.roughness;
                h = glm::mix(h, padHeight, mask * mask);
            }

            const glm::vec2 pondCenter(-46.0f, -74.0f);
            const glm::vec2 pondDelta = glm::vec2((wx - pondCenter.x) / 18.0f, (wz - pondCenter.y) / 24.0f);
            const float pondMask = glm::clamp(1.0f - glm::length(pondDelta), 0.0f, 1.0f);
            h -= pondMask * pondMask * 0.42f;

            const glm::vec2 colonyDip(-28.0f, -22.0f);
            const glm::vec2 colonyDelta = glm::vec2((wx - colonyDip.x) / 34.0f, (wz - colonyDip.y) / 26.0f);
            const float colonyMask = glm::clamp(1.0f - glm::length(colonyDelta), 0.0f, 1.0f);
            h -= colonyMask * 0.10f;

            const float edgeX = std::abs(wx) / (static_cast<float>(gridW) * cell * 0.5f);
            const float edgeZ = std::abs(wz) / (static_cast<float>(gridD) * cell * 0.5f);
            const float edge = std::max(edgeX, edgeZ);
            if (edge > 0.70f) {
                const float edgeRise = glm::smoothstep(0.70f, 1.0f, edge);
                h += edgeRise * 0.9f + edgeRise * edgeRise * 0.7f;
            }

            heightMap[static_cast<size_t>(z * gridW + x)] = h;
        }
    }
}

void Terrain::BuildMesh() {
    verts.clear();
    inds.clear();

    verts.reserve(static_cast<size_t>(gridW * gridD));
    for (int z = 0; z < gridD; ++z) {
        for (int x = 0; x < gridW; ++x) {
            TerrainVert v{};
            v.pos.x = (static_cast<float>(x) - static_cast<float>(gridW) * 0.5f) * cell;
            v.pos.z = (static_cast<float>(z) - static_cast<float>(gridD) * 0.5f) * cell;
            v.pos.y = heightMap[static_cast<size_t>(z * gridW + x)];
            v.nrm = glm::vec3(0.0f, 1.0f, 0.0f);
            v.uv = glm::vec2(static_cast<float>(x) * 0.32f, static_cast<float>(z) * 0.32f);
            verts.push_back(v);
        }
    }

    auto sampleH = [&](int x, int z) {
        x = std::clamp(x, 0, gridW - 1);
        z = std::clamp(z, 0, gridD - 1);
        return heightMap[static_cast<size_t>(z * gridW + x)];
    };

    for (int z = 0; z < gridD; ++z) {
        for (int x = 0; x < gridW; ++x) {
            const float hl = sampleH(x - 1, z);
            const float hr = sampleH(x + 1, z);
            const float hd = sampleH(x, z - 1);
            const float hu = sampleH(x, z + 1);
            glm::vec3 n = glm::normalize(glm::vec3(hl - hr, 2.0f * cell, hd - hu));
            verts[static_cast<size_t>(z * gridW + x)].nrm = n;
        }
    }

    for (int z = 0; z < gridD - 1; ++z) {
        for (int x = 0; x < gridW - 1; ++x) {
            const unsigned int i0 = static_cast<unsigned int>(z * gridW + x);
            const unsigned int i1 = static_cast<unsigned int>(z * gridW + x + 1);
            const unsigned int i2 = static_cast<unsigned int>((z + 1) * gridW + x);
            const unsigned int i3 = static_cast<unsigned int>((z + 1) * gridW + x + 1);

            inds.push_back(i0); inds.push_back(i2); inds.push_back(i1);
            inds.push_back(i1); inds.push_back(i2); inds.push_back(i3);
        }
    }

    if (vao) glDeleteVertexArrays(1, &vao);
    if (vbo) glDeleteBuffers(1, &vbo);
    if (ebo) glDeleteBuffers(1, &ebo);

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(verts.size() * sizeof(TerrainVert)), verts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(inds.size() * sizeof(unsigned int)), inds.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(TerrainVert), reinterpret_cast<void*>(offsetof(TerrainVert, pos)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(TerrainVert), reinterpret_cast<void*>(offsetof(TerrainVert, nrm)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(TerrainVert), reinterpret_cast<void*>(offsetof(TerrainVert, uv)));

    glBindVertexArray(0);
    indexCount = static_cast<int>(inds.size());
}
