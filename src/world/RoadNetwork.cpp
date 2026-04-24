#include "RoadNetwork.h"

#include <algorithm>
#include <cmath>

#include <glm/gtc/constants.hpp>

#include "renderer/Shader.h"
#include "world/Terrain.h"

RoadNetwork::~RoadNetwork() {
    ClearBatches();
}

void RoadNetwork::Build(Terrain& terrain) {
    segments.clear();
    ClearBatches();

    std::vector<RoadVert> tarmacVerts;
    std::vector<unsigned int> tarmacInds;
    std::vector<RoadVert> gravelVerts;
    std::vector<unsigned int> gravelInds;
    std::vector<RoadVert> dirtVerts;
    std::vector<unsigned int> dirtInds;
    std::vector<RoadVert> markingVerts;
    std::vector<unsigned int> markingInds;

    segments.push_back({glm::vec3(0.0f, 0.0f, -320.0f), glm::vec3(0.0f, 0.0f, 320.0f), 8.0f, RoadType::TARMAC});
    segments.push_back({glm::vec3(-300.0f, 0.0f, 0.0f), glm::vec3(300.0f, 0.0f, 0.0f), 8.0f, RoadType::TARMAC});
    segments.push_back({glm::vec3(-80.0f, 0.0f, -60.0f), glm::vec3(80.0f, 0.0f, -60.0f), 6.0f, RoadType::TARMAC});
    segments.push_back({glm::vec3(-80.0f, 0.0f, 60.0f), glm::vec3(80.0f, 0.0f, 60.0f), 6.0f, RoadType::TARMAC});
    segments.push_back({glm::vec3(-80.0f, 0.0f, -60.0f), glm::vec3(-80.0f, 0.0f, 60.0f), 6.0f, RoadType::TARMAC});
    segments.push_back({glm::vec3(80.0f, 0.0f, -60.0f), glm::vec3(80.0f, 0.0f, 60.0f), 6.0f, RoadType::TARMAC});
    segments.push_back({glm::vec3(-82.0f, 0.0f, -24.0f), glm::vec3(-12.0f, 0.0f, -8.0f), 4.8f, RoadType::GRAVEL});
    segments.push_back({glm::vec3(4.0f, 0.0f, -18.0f), glm::vec3(28.0f, 0.0f, -18.0f), 4.2f, RoadType::GRAVEL});
    segments.push_back({glm::vec3(22.0f, 0.0f, -24.0f), glm::vec3(42.0f, 0.0f, -68.0f), 4.0f, RoadType::DIRT});
    segments.push_back({glm::vec3(28.0f, 0.0f, 28.0f), glm::vec3(74.0f, 0.0f, 74.0f), 4.5f, RoadType::GRAVEL});
    segments.push_back({glm::vec3(36.0f, 0.0f, 22.0f), glm::vec3(58.0f, 0.0f, 38.0f), 4.0f, RoadType::GRAVEL});
    segments.push_back({glm::vec3(-22.0f, 0.0f, 34.0f), glm::vec3(-76.0f, 0.0f, 56.0f), 4.5f, RoadType::GRAVEL});
    segments.push_back({glm::vec3(-18.0f, 0.0f, 40.0f), glm::vec3(-18.0f, 0.0f, 128.0f), 4.0f, RoadType::DIRT});
    segments.push_back({glm::vec3(62.0f, 0.0f, -18.0f), glm::vec3(114.0f, 0.0f, -20.0f), 4.4f, RoadType::GRAVEL});
    segments.push_back({glm::vec3(84.0f, 0.0f, -20.0f), glm::vec3(84.0f, 0.0f, 30.0f), 4.0f, RoadType::GRAVEL});
    segments.push_back({glm::vec3(-150.0f, 0.0f, -110.0f), glm::vec3(-98.0f, 0.0f, -72.0f), 3.8f, RoadType::DIRT});
    segments.push_back({glm::vec3(90.0f, 0.0f, 94.0f), glm::vec3(124.0f, 0.0f, 122.0f), 3.8f, RoadType::DIRT});
    segments.push_back({glm::vec3(-108.0f, 0.0f, 12.0f), glm::vec3(-68.0f, 0.0f, 10.0f), 5.2f, RoadType::DIRT});
    segments.push_back({glm::vec3(-70.0f, 0.0f, 10.0f), glm::vec3(-34.0f, 0.0f, -22.0f), 4.6f, RoadType::GRAVEL});
    segments.push_back({glm::vec3(-8.0f, 0.0f, 84.0f), glm::vec3(42.0f, 0.0f, 84.0f), 5.0f, RoadType::TARMAC});
    segments.push_back({glm::vec3(42.0f, 0.0f, 84.0f), glm::vec3(78.0f, 0.0f, 74.0f), 4.8f, RoadType::GRAVEL});
    segments.push_back({glm::vec3(78.0f, 0.0f, 74.0f), glm::vec3(104.0f, 0.0f, 54.0f), 4.6f, RoadType::TARMAC});
    segments.push_back({glm::vec3(-58.0f, 0.0f, 48.0f), glm::vec3(-82.0f, 0.0f, 54.0f), 4.4f, RoadType::GRAVEL});

    for (const RoadSegment& seg : segments) {
        std::vector<RoadVert>* targetVerts = &gravelVerts;
        std::vector<unsigned int>* targetInds = &gravelInds;

        if (seg.type == RoadType::TARMAC) {
            GenerateSegmentMesh({seg.start, seg.end, seg.width + 1.4f, RoadType::GRAVEL}, gravelVerts, gravelInds);
            targetVerts = &tarmacVerts;
            targetInds = &tarmacInds;
            GenerateRoadMarkings(seg, markingVerts, markingInds);
        } else if (seg.type == RoadType::DIRT) {
            targetVerts = &dirtVerts;
            targetInds = &dirtInds;
        }

        GenerateSegmentMesh(seg, *targetVerts, *targetInds);
    }

    ApplyTerrainHeights(terrain, gravelVerts, 0.01f);
    ApplyTerrainHeights(terrain, dirtVerts, 0.015f);
    ApplyTerrainHeights(terrain, tarmacVerts, 0.03f);
    ApplyTerrainHeights(terrain, markingVerts, 0.038f);

    UploadBatch(gravelVerts, gravelInds, glm::vec3(0.34f, 0.32f, 0.29f), 0.92f);
    UploadBatch(dirtVerts, dirtInds, glm::vec3(0.28f, 0.23f, 0.18f), 0.96f);
    UploadBatch(tarmacVerts, tarmacInds, glm::vec3(0.17f, 0.17f, 0.16f), 0.86f);
    UploadBatch(markingVerts, markingInds, glm::vec3(0.74f, 0.67f, 0.30f), 0.60f);
}

void RoadNetwork::Draw(Shader& shader) const {
    for (const RoadBatch& batch : batches) {
        if (!batch.vao || batch.indexCount <= 0) {
            continue;
        }

        shader.SetMat4("model", glm::mat4(1.0f));
        shader.SetBool("useTexture", false);
        shader.SetVec3("albedoColor", batch.albedo);
        shader.SetFloat("roughness", batch.roughness);
        shader.SetFloat("metalness", 0.0f);
        shader.SetVec3("emission", glm::vec3(0.0f));

        glBindVertexArray(batch.vao);
        glDrawElements(GL_TRIANGLES, batch.indexCount, GL_UNSIGNED_INT, nullptr);
    }

    glBindVertexArray(0);
}

void RoadNetwork::ClearBatches() {
    for (RoadBatch& batch : batches) {
        if (batch.ebo) glDeleteBuffers(1, &batch.ebo);
        if (batch.vbo) glDeleteBuffers(1, &batch.vbo);
        if (batch.vao) glDeleteVertexArrays(1, &batch.vao);
        batch = RoadBatch{};
    }
    batches.clear();
}

void RoadNetwork::GenerateSegmentMesh(const RoadSegment& seg, std::vector<RoadVert>& verts, std::vector<unsigned int>& inds) {
    const glm::vec3 direction = seg.end - seg.start;
    if (glm::dot(direction, direction) < 1e-6f) {
        return;
    }

    const glm::vec3 d = glm::normalize(direction);
    const glm::vec3 right = glm::normalize(glm::cross(d, glm::vec3(0.0f, 1.0f, 0.0f)));
    const glm::vec3 off = right * (seg.width * 0.5f);

    const glm::vec3 p0 = seg.start - off;
    const glm::vec3 p1 = seg.start + off;
    const glm::vec3 p2 = seg.end - off;
    const glm::vec3 p3 = seg.end + off;

    const unsigned int base = static_cast<unsigned int>(verts.size());
    const float length = glm::length(seg.end - seg.start);

    verts.push_back({p0, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 0.0f)});
    verts.push_back({p1, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(seg.width * 0.25f, 0.0f)});
    verts.push_back({p2, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, length * 0.18f)});
    verts.push_back({p3, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(seg.width * 0.25f, length * 0.18f)});

    inds.push_back(base + 0); inds.push_back(base + 2); inds.push_back(base + 1);
    inds.push_back(base + 1); inds.push_back(base + 2); inds.push_back(base + 3);
}

void RoadNetwork::GenerateRoadMarkings(const RoadSegment& seg, std::vector<RoadVert>& verts, std::vector<unsigned int>& inds) {
    if (seg.type != RoadType::TARMAC || seg.width < 6.0f) {
        return;
    }

    const glm::vec3 direction = seg.end - seg.start;
    const float totalLength = glm::length(direction);
    if (totalLength < 0.1f) {
        return;
    }

    const glm::vec3 d = glm::normalize(direction);
    const glm::vec3 right = glm::normalize(glm::cross(d, glm::vec3(0.0f, 1.0f, 0.0f)));
    const float dashLength = 3.0f;
    const float gapLength = 2.0f;
    const float halfWidth = 0.13f;

    for (float cursor = 0.0f; cursor < totalLength; cursor += dashLength + gapLength) {
        const float next = std::min(cursor + dashLength, totalLength);
        if (next - cursor < 0.6f) {
            continue;
        }

        const glm::vec3 start = seg.start + d * cursor;
        const glm::vec3 end = seg.start + d * next;
        const glm::vec3 off = right * halfWidth;

        const unsigned int base = static_cast<unsigned int>(verts.size());
        verts.push_back({start - off, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, cursor)});
        verts.push_back({start + off, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, cursor)});
        verts.push_back({end - off, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, next)});
        verts.push_back({end + off, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, next)});

        inds.push_back(base + 0); inds.push_back(base + 2); inds.push_back(base + 1);
        inds.push_back(base + 1); inds.push_back(base + 2); inds.push_back(base + 3);
    }
}

void RoadNetwork::ApplyTerrainHeights(Terrain& terrain, std::vector<RoadVert>& verts, float heightBias) {
    for (RoadVert& vert : verts) {
        vert.pos.y = terrain.GetHeightAt(vert.pos.x, vert.pos.z) + heightBias;
    }
}

void RoadNetwork::UploadBatch(const std::vector<RoadVert>& verts,
                              const std::vector<unsigned int>& inds,
                              glm::vec3 albedo,
                              float roughness) {
    if (verts.empty() || inds.empty()) {
        return;
    }

    RoadBatch batch;
    batch.albedo = albedo;
    batch.roughness = roughness;
    batch.indexCount = static_cast<int>(inds.size());

    glGenVertexArrays(1, &batch.vao);
    glGenBuffers(1, &batch.vbo);
    glGenBuffers(1, &batch.ebo);

    glBindVertexArray(batch.vao);
    glBindBuffer(GL_ARRAY_BUFFER, batch.vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(verts.size() * sizeof(RoadVert)), verts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batch.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(inds.size() * sizeof(unsigned int)), inds.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(RoadVert), reinterpret_cast<void*>(offsetof(RoadVert, pos)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(RoadVert), reinterpret_cast<void*>(offsetof(RoadVert, nrm)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(RoadVert), reinterpret_cast<void*>(offsetof(RoadVert, uv)));

    glBindVertexArray(0);
    batches.push_back(batch);
}
