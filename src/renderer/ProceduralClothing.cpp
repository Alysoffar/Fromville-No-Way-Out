#include "ProceduralClothing.h"

#include "renderer/ProceduralHumanoid.h"
#include "renderer/Shader.h"

namespace {

void PushBox(std::vector<ProceduralClothing::ClothingVert>& verts,
             std::vector<unsigned int>& inds,
             glm::vec3 center,
             glm::vec3 dims,
             glm::vec3 color) {
    const glm::vec3 h = dims * 0.5f;
    const glm::vec3 p[8] = {
        {-h.x, -h.y, -h.z}, {h.x, -h.y, -h.z}, {h.x, h.y, -h.z}, {-h.x, h.y, -h.z},
        {-h.x, -h.y, h.z},  {h.x, -h.y, h.z},  {h.x, h.y, h.z},  {-h.x, h.y, h.z},
    };

    const unsigned int tri[36] = {
        0, 1, 2, 2, 3, 0,
        4, 6, 5, 6, 4, 7,
        0, 4, 5, 5, 1, 0,
        3, 2, 6, 6, 7, 3,
        1, 5, 6, 6, 2, 1,
        0, 3, 7, 7, 4, 0,
    };

    const unsigned int base = static_cast<unsigned int>(verts.size());
    for (int i = 0; i < 8; ++i) {
        ProceduralClothing::ClothingVert v{};
        v.pos = center + p[i];
        v.nrm = glm::vec3(0.0f, 1.0f, 0.0f);
        v.uv = glm::vec2((p[i].x / dims.x) + 0.5f, (p[i].y / dims.y) + 0.5f);
        v.color = color;
        verts.push_back(v);
    }
    for (int i = 0; i < 36; ++i) {
        inds.push_back(base + tri[i]);
    }
}

} // namespace

ProceduralClothing::ProceduralClothing(ClothingLayer shirtStyle, TrouserStyle trouserStyle)
    : shirt(shirtStyle), trousers(trouserStyle) {}

ProceduralClothing::~ProceduralClothing() {
    if (shirtEBO) glDeleteBuffers(1, &shirtEBO);
    if (shirtVBO) glDeleteBuffers(1, &shirtVBO);
    if (shirtVAO) glDeleteVertexArrays(1, &shirtVAO);
    if (trouserEBO) glDeleteBuffers(1, &trouserEBO);
    if (trouserVBO) glDeleteBuffers(1, &trouserVBO);
    if (trouserVAO) glDeleteVertexArrays(1, &trouserVAO);
}

void ProceduralClothing::Build(ProceduralHumanoid* body) {
    (void)body;
    BuildShirt();
    BuildTrousers();

    UploadMesh(shirtVAO, shirtVBO, shirtEBO, shirtVerts, shirtInds, shirtIndexCount);
    UploadMesh(trouserVAO, trouserVBO, trouserEBO, trouserVerts, trouserInds, trouserIndexCount);
}

void ProceduralClothing::Draw(Shader& shader, const glm::mat4& rootTransform) const {
    shader.SetMat4("model", rootTransform);
    shader.SetFloat("roughness", 0.82f);
    shader.SetFloat("metalness", 0.0f);
    shader.SetVec3("emission", glm::vec3(0.0f));

    if (shirtVAO && shirtIndexCount > 0) {
        glm::vec3 shirtColor(0.22f, 0.26f, 0.24f);
        if (shirt == ClothingLayer::CREATURE_JACKET) {
            shirtColor = glm::vec3(0.16f, 0.21f, 0.18f);
        } else if (shirt == ClothingLayer::UNIFORM_SHIRT) {
            shirtColor = glm::vec3(0.24f, 0.23f, 0.20f);
        }
        shader.SetVec3("albedoColor", shirtColor);
        glBindVertexArray(shirtVAO);
        glDrawElements(GL_TRIANGLES, shirtIndexCount, GL_UNSIGNED_INT, nullptr);
    }

    if (trouserVAO && trouserIndexCount > 0) {
        glm::vec3 trouserColor(0.16f, 0.16f, 0.18f);
        if (shirt == ClothingLayer::CREATURE_JACKET) {
            trouserColor = glm::vec3(0.11f, 0.11f, 0.12f);
        }
        shader.SetVec3("albedoColor", trouserColor);
        glBindVertexArray(trouserVAO);
        glDrawElements(GL_TRIANGLES, trouserIndexCount, GL_UNSIGNED_INT, nullptr);
    }

    glBindVertexArray(0);
}

void ProceduralClothing::BuildShirt() {
    shirtVerts.clear();
    shirtInds.clear();

    float inflate = 0.015f;
    if (shirt == ClothingLayer::HOODIE) inflate = 0.020f;
    if (shirt == ClothingLayer::UNIFORM_SHIRT) inflate = 0.012f;
    if (shirt == ClothingLayer::CREATURE_JACKET) inflate = 0.028f;

    PushBox(shirtVerts, shirtInds, glm::vec3(0.0f, 0.96f, 0.0f), glm::vec3(0.56f + inflate, 0.62f, 0.34f + inflate), glm::vec3(0.2f));

    if (shirt == ClothingLayer::HOODIE) {
        PushBox(shirtVerts, shirtInds, glm::vec3(0.0f, 0.78f, 0.17f), glm::vec3(0.24f, 0.11f, 0.03f), glm::vec3(0.2f));
    }
    if (shirt == ClothingLayer::UNIFORM_SHIRT) {
        PushBox(shirtVerts, shirtInds, glm::vec3(-0.08f, 1.08f, 0.19f), glm::vec3(0.09f, 0.07f, 0.015f), glm::vec3(0.2f));
    }
    if (shirt == ClothingLayer::CREATURE_JACKET) {
        PushBox(shirtVerts, shirtInds, glm::vec3(0.0f, 1.08f, 0.0f), glm::vec3(0.66f, 0.22f, 0.42f), glm::vec3(0.2f));
        PushBox(shirtVerts, shirtInds, glm::vec3(0.0f, 0.90f, 0.20f), glm::vec3(0.28f, 0.18f, 0.08f), glm::vec3(0.2f));
    }
}

void ProceduralClothing::BuildTrousers() {
    trouserVerts.clear();
    trouserInds.clear();

    PushBox(trouserVerts, trouserInds, glm::vec3(-0.12f, 0.44f, 0.0f), glm::vec3(0.20f, 0.74f, 0.20f), glm::vec3(0.1f));
    PushBox(trouserVerts, trouserInds, glm::vec3(0.12f, 0.44f, 0.0f), glm::vec3(0.20f, 0.74f, 0.20f), glm::vec3(0.1f));
}

void ProceduralClothing::UploadMesh(GLuint& vao, GLuint& vbo, GLuint& ebo, const std::vector<ClothingVert>& verts, const std::vector<unsigned int>& inds, int& outIndexCount) {
    if (vao) glDeleteVertexArrays(1, &vao);
    if (vbo) glDeleteBuffers(1, &vbo);
    if (ebo) glDeleteBuffers(1, &ebo);

    if (verts.empty() || inds.empty()) {
        vao = vbo = ebo = 0;
        outIndexCount = 0;
        return;
    }

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(verts.size() * sizeof(ClothingVert)), verts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(inds.size() * sizeof(unsigned int)), inds.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ClothingVert), reinterpret_cast<void*>(offsetof(ClothingVert, pos)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ClothingVert), reinterpret_cast<void*>(offsetof(ClothingVert, nrm)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(ClothingVert), reinterpret_cast<void*>(offsetof(ClothingVert, uv)));

    glBindVertexArray(0);
    outIndexCount = static_cast<int>(inds.size());
}
