#include "FaceDetailGenerator.h"

#include "renderer/ProceduralHumanoid.h"
#include "renderer/Shader.h"

FaceDetailGenerator::FaceDetailGenerator() = default;

FaceDetailGenerator::~FaceDetailGenerator() {
    if (ebo) glDeleteBuffers(1, &ebo);
    if (vbo) glDeleteBuffers(1, &vbo);
    if (vao) glDeleteVertexArrays(1, &vao);
}

void FaceDetailGenerator::Build(ProceduralHumanoid* human, EyebrowStyle brow, LipStyle lip, NoseStyle nose) {
    (void)human;
    verts.clear();
    inds.clear();

    float browWidth = 0.08f;
    if (brow == EyebrowStyle::THIN_LIGHT) browWidth = 0.065f;
    if (brow == EyebrowStyle::HEAVY_FLAT) browWidth = 0.095f;

    float lipWidth = 0.08f;
    if (lip == LipStyle::FULL) lipWidth = 0.095f;
    if (lip == LipStyle::NARROW) lipWidth = 0.065f;

    float noseHeight = 0.05f;
    if (nose == NoseStyle::NARROW_LONG) noseHeight = 0.065f;
    if (nose == NoseStyle::SMALL_UPTURNED) noseHeight = 0.042f;

    PushQuad(glm::vec3(-0.045f, 1.61f, 0.102f), glm::vec2(browWidth, 0.012f), glm::vec3(0.0f, 0.0f, 1.0f));
    PushQuad(glm::vec3(0.045f, 1.61f, 0.102f), glm::vec2(browWidth, 0.012f), glm::vec3(0.0f, 0.0f, 1.0f));
    PushQuad(glm::vec3(0.0f, 1.49f, 0.104f), glm::vec2(lipWidth, 0.018f), glm::vec3(0.0f, 0.0f, 1.0f));
    PushQuad(glm::vec3(0.0f, 1.54f, 0.108f), glm::vec2(0.025f, noseHeight), glm::vec3(0.0f, 0.0f, 1.0f));

    Upload();
}

void FaceDetailGenerator::Draw(Shader& shader, const glm::mat4& rootTransform) const {
    if (!vao || indexCount <= 0) {
        return;
    }

    shader.SetMat4("model", rootTransform);
    shader.SetFloat("roughness", 0.75f);
    shader.SetFloat("metalness", 0.0f);
    shader.SetVec3("emission", glm::vec3(0.0f));
    shader.SetVec3("albedoColor", glm::vec3(0.22f, 0.11f, 0.10f));

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void FaceDetailGenerator::PushQuad(glm::vec3 center, glm::vec2 size, glm::vec3 normal) {
    unsigned int base = static_cast<unsigned int>(verts.size());
    const float hx = size.x * 0.5f;
    const float hy = size.y * 0.5f;

    FaceVert v0{}; v0.pos = center + glm::vec3(-hx, -hy, 0.0f); v0.nrm = normal; v0.uv = glm::vec2(0.0f, 0.0f);
    FaceVert v1{}; v1.pos = center + glm::vec3(hx, -hy, 0.0f);  v1.nrm = normal; v1.uv = glm::vec2(1.0f, 0.0f);
    FaceVert v2{}; v2.pos = center + glm::vec3(hx, hy, 0.0f);   v2.nrm = normal; v2.uv = glm::vec2(1.0f, 1.0f);
    FaceVert v3{}; v3.pos = center + glm::vec3(-hx, hy, 0.0f);  v3.nrm = normal; v3.uv = glm::vec2(0.0f, 1.0f);

    verts.push_back(v0);
    verts.push_back(v1);
    verts.push_back(v2);
    verts.push_back(v3);

    inds.push_back(base + 0);
    inds.push_back(base + 1);
    inds.push_back(base + 2);
    inds.push_back(base + 2);
    inds.push_back(base + 3);
    inds.push_back(base + 0);
}

void FaceDetailGenerator::Upload() {
    if (vao) glDeleteVertexArrays(1, &vao);
    if (vbo) glDeleteBuffers(1, &vbo);
    if (ebo) glDeleteBuffers(1, &ebo);

    if (verts.empty() || inds.empty()) {
        vao = vbo = ebo = 0;
        indexCount = 0;
        return;
    }

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(verts.size() * sizeof(FaceVert)), verts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(inds.size() * sizeof(unsigned int)), inds.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(FaceVert), reinterpret_cast<void*>(offsetof(FaceVert, pos)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(FaceVert), reinterpret_cast<void*>(offsetof(FaceVert, nrm)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(FaceVert), reinterpret_cast<void*>(offsetof(FaceVert, uv)));

    glBindVertexArray(0);
    indexCount = static_cast<int>(inds.size());
}
