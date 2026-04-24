#include "ProceduralHair.h"

#include <cmath>

#include <glm/gtc/constants.hpp>

#include "renderer/ProceduralHumanoid.h"
#include "renderer/Shader.h"

ProceduralHair::ProceduralHair(HairStyle hairStyle, glm::vec3 base, glm::vec3 highlight, float strandLength)
    : style(hairStyle), baseColor(base), highlightColor(highlight), length(strandLength) {}

ProceduralHair::~ProceduralHair() {
    if (ebo) glDeleteBuffers(1, &ebo);
    if (vbo) glDeleteBuffers(1, &vbo);
    if (vao) glDeleteVertexArrays(1, &vao);
}

void ProceduralHair::GenerateStrands(ProceduralHumanoid* humanoid) {
    (void)humanoid;

    verts.clear();
    inds.clear();

    int strandCount = 700;
    int points = 4;
    if (style == HairStyle::CLOSE_CROP) {
        strandCount = 420;
        points = 3;
    } else if (style == HairStyle::CREATURE_MATTED) {
        strandCount = 980;
        points = 6;
    } else if (style == HairStyle::LONG_STRAIGHT) {
        strandCount = 1200;
        points = 8;
    } else if (style == HairStyle::PONYTAIL_LOOSE) {
        strandCount = 950;
        points = 7;
    }

    const float scalpR = 0.11f;
    const float w = (style == HairStyle::CREATURE_MATTED) ? 0.0029f : 0.0018f;

    for (int s = 0; s < strandCount; ++s) {
        const float u = static_cast<float>(s) / static_cast<float>(strandCount);
        const float a = u * glm::two_pi<float>();
        const float ring = 0.45f + 0.55f * std::sin(u * 11.0f);
        const glm::vec3 base = glm::vec3(std::cos(a) * scalpR * ring, 1.58f, std::sin(a) * scalpR * ring);

        int baseIndex = static_cast<int>(verts.size());
        for (int p = 0; p < points; ++p) {
            const float t = static_cast<float>(p) / static_cast<float>(points - 1);
            const float sway = std::sin(a * 2.0f + t * 4.0f) * 0.015f;
            glm::vec3 dir(0.0f, -1.0f, 0.0f);
            if (style == HairStyle::CREATURE_MATTED) {
                dir.x = std::sin(a * 3.0f) * 0.05f;
                dir.z = std::cos(a * 2.0f + t * 3.0f) * 0.05f;
            } else if (style == HairStyle::WAVY_MEDIUM || style == HairStyle::LONGISH_SWEPT) {
                dir.x = 0.08f;
                dir.z = sway;
            } else if (style == HairStyle::PONYTAIL_LOOSE) {
                dir.z = -0.12f;
                dir.x = sway * 0.8f;
            }

            float strandLength = length;
            if (style == HairStyle::CREATURE_MATTED) {
                strandLength *= 1.12f;
            }
            glm::vec3 center = base + dir * (strandLength * t);
            glm::vec3 side = glm::normalize(glm::vec3(-dir.z, 0.0f, dir.x));
            if (glm::length(side) < 1e-4f) {
                side = glm::vec3(1.0f, 0.0f, 0.0f);
            }

            HairVert left{};
            left.pos = center - side * w;
            left.nrm = glm::vec3(0.0f, 1.0f, 0.0f);
            left.uv = glm::vec2(0.0f, t);
            verts.push_back(left);

            HairVert right{};
            right.pos = center + side * w;
            right.nrm = glm::vec3(0.0f, 1.0f, 0.0f);
            right.uv = glm::vec2(1.0f, t);
            verts.push_back(right);

            if (p < points - 1) {
                int row = baseIndex + p * 2;
                inds.push_back(static_cast<unsigned int>(row + 0));
                inds.push_back(static_cast<unsigned int>(row + 1));
                inds.push_back(static_cast<unsigned int>(row + 2));

                inds.push_back(static_cast<unsigned int>(row + 1));
                inds.push_back(static_cast<unsigned int>(row + 3));
                inds.push_back(static_cast<unsigned int>(row + 2));
            }
        }
    }

    BuildMesh();
}

void ProceduralHair::Update(float dt, glm::vec3 characterVelocity) {
    (void)dt;
    (void)characterVelocity;
}

void ProceduralHair::Draw(Shader& shader, const glm::mat4& rootTransform) const {
    if (!vao || indexCount <= 0) {
        return;
    }

    shader.SetMat4("model", rootTransform);
    shader.SetVec3("albedoColor", glm::mix(baseColor, highlightColor, 0.2f));
    shader.SetFloat("roughness", 0.55f);
    shader.SetFloat("metalness", 0.0f);
    shader.SetVec3("emission", glm::vec3(0.0f));

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void ProceduralHair::BuildMesh() {
    if (vao) glDeleteVertexArrays(1, &vao);
    if (vbo) glDeleteBuffers(1, &vbo);
    if (ebo) glDeleteBuffers(1, &ebo);

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(verts.size() * sizeof(HairVert)), verts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(inds.size() * sizeof(unsigned int)), inds.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(HairVert), reinterpret_cast<void*>(offsetof(HairVert, pos)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(HairVert), reinterpret_cast<void*>(offsetof(HairVert, nrm)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(HairVert), reinterpret_cast<void*>(offsetof(HairVert, uv)));

    glBindVertexArray(0);
    indexCount = static_cast<int>(inds.size());
}
