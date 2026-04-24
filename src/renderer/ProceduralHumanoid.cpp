#include "ProceduralHumanoid.h"

#include <algorithm>
#include <cmath>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "renderer/Shader.h"

namespace {
constexpr float kEpsilon = 1e-5f;

glm::vec3 SafeNormalize(const glm::vec3& v) {
    const float len = glm::length(v);
    if (len < kEpsilon) {
        return glm::vec3(0.0f, 1.0f, 0.0f);
    }
    return v / len;
}

} // namespace

ProceduralHumanoid::ProceduralHumanoid(const BuildParams& buildParams)
    : params(buildParams) {
    Build();
}

ProceduralHumanoid::~ProceduralHumanoid() {
    for (auto& it : segments) {
        BodySegment& seg = it.second;
        if (seg.EBO != 0) {
            glDeleteBuffers(1, &seg.EBO);
            seg.EBO = 0;
        }
        if (seg.VBO != 0) {
            glDeleteBuffers(1, &seg.VBO);
            seg.VBO = 0;
        }
        if (seg.VAO != 0) {
            glDeleteVertexArrays(1, &seg.VAO);
            seg.VAO = 0;
        }
    }
}

void ProceduralHumanoid::Build() {
    segments.clear();
    jointLocalTransforms.clear();
    jointWorldTransforms.clear();
    parentJoints.clear();
    jointToBoneIndex.clear();

    BuildSkeleton();
    BuildDefaultSegments();
    ComputeForwardKinematics();
}

void ProceduralHumanoid::Draw(Shader& shader, const std::vector<glm::mat4>& boneTransforms) {
    const glm::mat4 root = GetJointWorldTransform(rootJoint);

    for (const auto& it : segments) {
        const JointId joint = it.first;
        const BodySegment& seg = it.second;

        glm::mat4 model = root * GetJointWorldTransform(joint);
        auto boneIt = jointToBoneIndex.find(joint);
        if (boneIt != jointToBoneIndex.end()) {
            const int idx = boneIt->second;
            if (idx >= 0 && static_cast<size_t>(idx) < boneTransforms.size()) {
                model = boneTransforms[static_cast<size_t>(idx)] * model;
            }
        }

        shader.SetMat4("model", model);
        glBindVertexArray(seg.VAO);
        glDrawElements(GL_TRIANGLES, seg.indexCount, GL_UNSIGNED_INT, nullptr);
    }
    glBindVertexArray(0);
}

void ProceduralHumanoid::DrawSegment(JointId joint, Shader& shader, const glm::mat4& rootTransform) {
    auto it = segments.find(joint);
    if (it == segments.end()) {
        return;
    }

    const BodySegment& seg = it->second;
    const glm::mat4 model = rootTransform * GetJointWorldTransform(joint);
    shader.SetMat4("model", model);

    glBindVertexArray(seg.VAO);
    glDrawElements(GL_TRIANGLES, seg.indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void ProceduralHumanoid::UpdateJointTransform(JointId joint, const glm::mat4& localTransform) {
    jointLocalTransforms[joint] = localTransform;
    ComputeForwardKinematics();
}

glm::mat4 ProceduralHumanoid::GetJointWorldTransform(JointId joint) const {
    auto it = jointWorldTransforms.find(joint);
    if (it != jointWorldTransforms.end()) {
        return it->second;
    }
    return glm::mat4(1.0f);
}

void ProceduralHumanoid::SetPose(const std::string& poseName) {
    if (poseName == "crouch") {
        jointLocalTransforms[JointId::UPPER_LEG_L] = glm::rotate(glm::mat4(1.0f), glm::radians(-18.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        jointLocalTransforms[JointId::UPPER_LEG_R] = glm::rotate(glm::mat4(1.0f), glm::radians(-18.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        jointLocalTransforms[JointId::LOWER_LEG_L] = glm::rotate(glm::mat4(1.0f), glm::radians(28.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        jointLocalTransforms[JointId::LOWER_LEG_R] = glm::rotate(glm::mat4(1.0f), glm::radians(28.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    } else {
        for (auto& it : jointLocalTransforms) {
            it.second = glm::mat4(1.0f);
        }
    }

    ComputeForwardKinematics();
}

void ProceduralHumanoid::BuildSkeleton() {
    const auto bind = [&](JointId child, JointId parent, int boneIndex) {
        parentJoints[child] = parent;
        jointLocalTransforms[child] = glm::mat4(1.0f);
        jointWorldTransforms[child] = glm::mat4(1.0f);
        jointToBoneIndex[child] = boneIndex;
    };

    bind(JointId::ROOT, JointId::ROOT, 0);
    bind(JointId::SPINE_1, JointId::ROOT, 1);
    bind(JointId::SPINE_2, JointId::SPINE_1, 2);
    bind(JointId::SPINE_3, JointId::SPINE_2, 3);
    bind(JointId::NECK, JointId::SPINE_3, 4);
    bind(JointId::HEAD, JointId::NECK, 5);
    bind(JointId::JAW, JointId::HEAD, 6);
    bind(JointId::EYE_L, JointId::HEAD, 7);
    bind(JointId::EYE_R, JointId::HEAD, 8);
    bind(JointId::SHOULDER_L, JointId::SPINE_3, 9);
    bind(JointId::UPPER_ARM_L, JointId::SHOULDER_L, 10);
    bind(JointId::LOWER_ARM_L, JointId::UPPER_ARM_L, 11);
    bind(JointId::HAND_L, JointId::LOWER_ARM_L, 12);
    bind(JointId::SHOULDER_R, JointId::SPINE_3, 13);
    bind(JointId::UPPER_ARM_R, JointId::SHOULDER_R, 14);
    bind(JointId::LOWER_ARM_R, JointId::UPPER_ARM_R, 15);
    bind(JointId::HAND_R, JointId::LOWER_ARM_R, 16);
    bind(JointId::HIP_L, JointId::ROOT, 17);
    bind(JointId::UPPER_LEG_L, JointId::HIP_L, 18);
    bind(JointId::LOWER_LEG_L, JointId::UPPER_LEG_L, 19);
    bind(JointId::FOOT_L, JointId::LOWER_LEG_L, 20);
    bind(JointId::TOE_L, JointId::FOOT_L, 21);
    bind(JointId::HIP_R, JointId::ROOT, 22);
    bind(JointId::UPPER_LEG_R, JointId::HIP_R, 23);
    bind(JointId::LOWER_LEG_R, JointId::UPPER_LEG_R, 24);
    bind(JointId::FOOT_R, JointId::LOWER_LEG_R, 25);
    bind(JointId::TOE_R, JointId::FOOT_R, 26);
}

void ProceduralHumanoid::BuildDefaultSegments() {
    const float h = params.heightMeters;
    const float torsoH = h * 0.32f;
    const float legH = h * 0.26f * params.limbLengthScale;
    const float calfH = h * 0.25f * params.limbLengthScale;
    const float armH = h * 0.17f * params.limbLengthScale;
    const float forearmH = h * 0.15f * params.limbLengthScale;

    const float shoulderHalf = params.shoulderWidthMeters * 0.5f;
    const float hipHalf = params.hipWidthMeters * 0.5f;
    const float muscle = glm::clamp(params.muscleScale, 0.6f, 1.4f);

    AddSegment(MakeCylinder(JointId::SPINE_1, glm::vec3(0.0f, h * 0.50f, 0.0f), 0.13f * muscle, 0.11f * muscle, torsoH, 20, 8));
    AddSegment(MakeSphere(JointId::HEAD, glm::vec3(0.0f, h * 0.88f, 0.0f), 0.105f * params.headSizeScale, 18, 20));
    AddSegment(MakeCylinder(JointId::NECK, glm::vec3(0.0f, h * 0.79f, 0.0f), 0.05f, 0.055f, h * 0.07f, 14, 4));

    AddSegment(MakeCapsule(JointId::UPPER_ARM_L, glm::vec3(-shoulderHalf, h * 0.66f, 0.0f), 0.045f * muscle, 0.04f * muscle, armH, 14, 6));
    AddSegment(MakeCapsule(JointId::LOWER_ARM_L, glm::vec3(-shoulderHalf, h * 0.53f, 0.0f), 0.038f * muscle, 0.032f * muscle, forearmH, 12, 5));
    AddSegment(MakeBox(JointId::HAND_L, glm::vec3(-shoulderHalf, h * 0.44f, 0.02f), glm::vec3(0.085f, 0.03f, 0.07f)));

    AddSegment(MakeCapsule(JointId::UPPER_ARM_R, glm::vec3(shoulderHalf, h * 0.66f, 0.0f), 0.045f * muscle, 0.04f * muscle, armH, 14, 6));
    AddSegment(MakeCapsule(JointId::LOWER_ARM_R, glm::vec3(shoulderHalf, h * 0.53f, 0.0f), 0.038f * muscle, 0.032f * muscle, forearmH, 12, 5));
    AddSegment(MakeBox(JointId::HAND_R, glm::vec3(shoulderHalf, h * 0.44f, 0.02f), glm::vec3(0.085f, 0.03f, 0.07f)));

    AddSegment(MakeCapsule(JointId::UPPER_LEG_L, glm::vec3(-hipHalf, h * 0.31f, 0.0f), 0.058f * muscle, 0.048f * muscle, legH, 16, 7));
    AddSegment(MakeCylinder(JointId::LOWER_LEG_L, glm::vec3(-hipHalf, h * 0.16f, 0.0f), 0.045f * muscle, 0.03f * muscle, calfH, 14, 7));
    AddSegment(MakeBox(JointId::FOOT_L, glm::vec3(-hipHalf, h * 0.04f, 0.09f), glm::vec3(0.095f, 0.05f, 0.23f)));

    AddSegment(MakeCapsule(JointId::UPPER_LEG_R, glm::vec3(hipHalf, h * 0.31f, 0.0f), 0.058f * muscle, 0.048f * muscle, legH, 16, 7));
    AddSegment(MakeCylinder(JointId::LOWER_LEG_R, glm::vec3(hipHalf, h * 0.16f, 0.0f), 0.045f * muscle, 0.03f * muscle, calfH, 14, 7));
    AddSegment(MakeBox(JointId::FOOT_R, glm::vec3(hipHalf, h * 0.04f, 0.09f), glm::vec3(0.095f, 0.05f, 0.23f)));
}

void ProceduralHumanoid::GenerateSegmentMesh(BodySegment& seg) {
    switch (seg.shape) {
    case SegmentShape::CAPSULE:
    case SegmentShape::SPHERE:
    case SegmentShape::BOX:
    case SegmentShape::CYLINDER:
    case SegmentShape::CONE:
    default:
        break;
    }

    ComputeNormals(seg.verts, seg.inds);
}

void ProceduralHumanoid::ComputeForwardKinematics() {
    for (const auto& it : jointLocalTransforms) {
        jointWorldTransforms[it.first] = it.second;
    }
}

BodySegment ProceduralHumanoid::MakeCapsule(JointId joint, glm::vec3 offset, float topR, float botR, float height, int sides, int segs) {
    return MakeCylinder(joint, offset, topR, botR, height, sides, segs);
}

BodySegment ProceduralHumanoid::MakeSphere(JointId joint, glm::vec3 offset, float radius, int latDiv, int lonDiv) {
    BodySegment seg;
    seg.joint = joint;
    seg.localOffset = offset;
    seg.shape = SegmentShape::SPHERE;
    seg.dimensions = glm::vec3(radius * 2.0f);
    seg.subdivisions = std::max(latDiv, lonDiv);

    latDiv = std::max(4, latDiv);
    lonDiv = std::max(6, lonDiv);

    for (int y = 0; y <= latDiv; ++y) {
        const float v = static_cast<float>(y) / static_cast<float>(latDiv);
        const float phi = v * glm::pi<float>();
        const float sy = std::cos(phi);
        const float sr = std::sin(phi);

        for (int x = 0; x <= lonDiv; ++x) {
            const float u = static_cast<float>(x) / static_cast<float>(lonDiv);
            const float theta = u * glm::two_pi<float>();

            const float sx = std::cos(theta) * sr;
            const float sz = std::sin(theta) * sr;

            ProceduralVertex vtx;
            vtx.position = glm::vec3(sx, sy, sz) * radius;
            vtx.uv = glm::vec2(u, 1.0f - v);
            seg.verts.push_back(vtx);
        }
    }

    for (int y = 0; y < latDiv; ++y) {
        for (int x = 0; x < lonDiv; ++x) {
            const int row0 = y * (lonDiv + 1);
            const int row1 = (y + 1) * (lonDiv + 1);

            seg.inds.push_back(static_cast<unsigned int>(row0 + x));
            seg.inds.push_back(static_cast<unsigned int>(row1 + x));
            seg.inds.push_back(static_cast<unsigned int>(row0 + x + 1));

            seg.inds.push_back(static_cast<unsigned int>(row0 + x + 1));
            seg.inds.push_back(static_cast<unsigned int>(row1 + x));
            seg.inds.push_back(static_cast<unsigned int>(row1 + x + 1));
        }
    }

    GenerateSegmentMesh(seg);
    UploadSegment(seg);
    return seg;
}

BodySegment ProceduralHumanoid::MakeBox(JointId joint, glm::vec3 offset, glm::vec3 dims) {
    BodySegment seg;
    seg.joint = joint;
    seg.localOffset = offset;
    seg.shape = SegmentShape::BOX;
    seg.dimensions = dims;
    seg.subdivisions = 1;

    const glm::vec3 h = dims * 0.5f;

    const glm::vec3 p[8] = {
        {-h.x, -h.y, -h.z}, {h.x, -h.y, -h.z}, {h.x, h.y, -h.z}, {-h.x, h.y, -h.z},
        {-h.x, -h.y, h.z},  {h.x, -h.y, h.z},  {h.x, h.y, h.z},  {-h.x, h.y, h.z},
    };

    const unsigned int faceIndices[36] = {
        0, 1, 2, 2, 3, 0,
        4, 6, 5, 6, 4, 7,
        0, 4, 5, 5, 1, 0,
        3, 2, 6, 6, 7, 3,
        1, 5, 6, 6, 2, 1,
        0, 3, 7, 7, 4, 0,
    };

    seg.verts.resize(8);
    for (int i = 0; i < 8; ++i) {
        seg.verts[i].position = p[i];
        seg.verts[i].uv = glm::vec2((p[i].x / dims.x) + 0.5f, (p[i].y / dims.y) + 0.5f);
    }
    seg.inds.assign(faceIndices, faceIndices + 36);

    GenerateSegmentMesh(seg);
    UploadSegment(seg);
    return seg;
}

BodySegment ProceduralHumanoid::MakeCylinder(JointId joint, glm::vec3 offset, float topR, float botR, float height, int sides, int segs) {
    BodySegment seg;
    seg.joint = joint;
    seg.localOffset = offset;
    seg.shape = SegmentShape::CYLINDER;
    seg.dimensions = glm::vec3(std::max(topR, botR) * 2.0f, height, std::max(topR, botR) * 2.0f);
    seg.subdivisions = sides;

    sides = std::max(6, sides);
    segs = std::max(1, segs);

    for (int y = 0; y <= segs; ++y) {
        const float t = static_cast<float>(y) / static_cast<float>(segs);
        const float radius = glm::mix(botR, topR, t);
        const float py = (t - 0.5f) * height;

        for (int i = 0; i <= sides; ++i) {
            const float u = static_cast<float>(i) / static_cast<float>(sides);
            const float a = u * glm::two_pi<float>();
            ProceduralVertex vtx;
            vtx.position = glm::vec3(std::cos(a) * radius, py, std::sin(a) * radius);
            vtx.uv = glm::vec2(u, t);
            seg.verts.push_back(vtx);
        }
    }

    for (int y = 0; y < segs; ++y) {
        const int row0 = y * (sides + 1);
        const int row1 = (y + 1) * (sides + 1);
        for (int i = 0; i < sides; ++i) {
            seg.inds.push_back(static_cast<unsigned int>(row0 + i));
            seg.inds.push_back(static_cast<unsigned int>(row1 + i));
            seg.inds.push_back(static_cast<unsigned int>(row0 + i + 1));

            seg.inds.push_back(static_cast<unsigned int>(row0 + i + 1));
            seg.inds.push_back(static_cast<unsigned int>(row1 + i));
            seg.inds.push_back(static_cast<unsigned int>(row1 + i + 1));
        }
    }

    GenerateSegmentMesh(seg);
    UploadSegment(seg);
    return seg;
}

BodySegment ProceduralHumanoid::MakeCone(JointId joint, glm::vec3 offset, float radius, float height, int sides, int segs) {
    return MakeCylinder(joint, offset, 0.0f, radius, height, sides, segs);
}

void ProceduralHumanoid::UploadSegment(BodySegment& seg) {
    if (seg.verts.empty() || seg.inds.empty()) {
        return;
    }

    glGenVertexArrays(1, &seg.VAO);
    glGenBuffers(1, &seg.VBO);
    glGenBuffers(1, &seg.EBO);

    glBindVertexArray(seg.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, seg.VBO);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(seg.verts.size() * sizeof(ProceduralVertex)), seg.verts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, seg.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(seg.inds.size() * sizeof(unsigned int)), seg.inds.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralVertex), reinterpret_cast<void*>(offsetof(ProceduralVertex, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralVertex), reinterpret_cast<void*>(offsetof(ProceduralVertex, normal)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(ProceduralVertex), reinterpret_cast<void*>(offsetof(ProceduralVertex, uv)));

    glBindVertexArray(0);

    seg.indexCount = static_cast<int>(seg.inds.size());
}

void ProceduralHumanoid::ComputeNormals(std::vector<ProceduralVertex>& verts, const std::vector<unsigned int>& inds) {
    for (auto& v : verts) {
        v.normal = glm::vec3(0.0f);
    }

    for (size_t i = 0; i + 2 < inds.size(); i += 3) {
        const unsigned int ia = inds[i];
        const unsigned int ib = inds[i + 1];
        const unsigned int ic = inds[i + 2];
        if (ia >= verts.size() || ib >= verts.size() || ic >= verts.size()) {
            continue;
        }

        const glm::vec3 a = verts[ia].position;
        const glm::vec3 b = verts[ib].position;
        const glm::vec3 c = verts[ic].position;
        const glm::vec3 n = SafeNormalize(glm::cross(b - a, c - a));

        verts[ia].normal += n;
        verts[ib].normal += n;
        verts[ic].normal += n;
    }

    for (auto& v : verts) {
        v.normal = SafeNormalize(v.normal);
    }
}

void ProceduralHumanoid::AddSegment(BodySegment seg) {
    const glm::mat4 world = glm::translate(glm::mat4(1.0f), seg.localOffset);
    jointLocalTransforms[seg.joint] = world;
    segments[seg.joint] = std::move(seg);
}
