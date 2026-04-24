#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <string>
#include <unordered_map>
#include <vector>

class Shader;

enum class JointId {
    ROOT = 0,
    SPINE_1,
    SPINE_2,
    SPINE_3,
    NECK,
    HEAD,
    JAW,
    EYE_L,
    EYE_R,
    SHOULDER_L,
    UPPER_ARM_L,
    LOWER_ARM_L,
    HAND_L,
    SHOULDER_R,
    UPPER_ARM_R,
    LOWER_ARM_R,
    HAND_R,
    HIP_L,
    UPPER_LEG_L,
    LOWER_LEG_L,
    FOOT_L,
    TOE_L,
    HIP_R,
    UPPER_LEG_R,
    LOWER_LEG_R,
    FOOT_R,
    TOE_R,
};

enum class SegmentShape {
    CAPSULE,
    SPHERE,
    BOX,
    CYLINDER,
    CONE,
};

struct ProceduralVertex {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 normal = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec2 uv = glm::vec2(0.0f);
};

struct BodySegment {
    JointId joint = JointId::ROOT;
    glm::vec3 localOffset = glm::vec3(0.0f);
    glm::vec3 dimensions = glm::vec3(0.1f);
    SegmentShape shape = SegmentShape::BOX;
    int subdivisions = 12;
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;
    int indexCount = 0;
    std::vector<ProceduralVertex> verts;
    std::vector<unsigned int> inds;
};

class ProceduralHumanoid {
public:
    struct BuildParams {
        float heightMeters = 1.78f;
        float shoulderWidthMeters = 0.42f;
        float hipWidthMeters = 0.34f;
        float headSizeScale = 1.0f;
        float limbLengthScale = 1.0f;
        float muscleScale = 1.0f;
        bool isFemale = false;
        glm::vec3 skinColor = glm::vec3(0.7f, 0.6f, 0.5f);
    };

    explicit ProceduralHumanoid(const BuildParams& buildParams);
    ~ProceduralHumanoid();

    void Build();
    void Draw(Shader& shader, const std::vector<glm::mat4>& boneTransforms);
    void DrawSegment(JointId joint, Shader& shader, const glm::mat4& rootTransform);
    void UpdateJointTransform(JointId joint, const glm::mat4& localTransform);
    glm::mat4 GetJointWorldTransform(JointId joint) const;
    void SetPose(const std::string& poseName);

    const BuildParams& GetBuildParams() const { return params; }
    const glm::vec3& GetSkinColor() const { return params.skinColor; }

private:
    BuildParams params;
    std::unordered_map<JointId, BodySegment> segments;
    std::unordered_map<JointId, glm::mat4> jointLocalTransforms;
    std::unordered_map<JointId, glm::mat4> jointWorldTransforms;
    std::unordered_map<JointId, JointId> parentJoints;
    std::unordered_map<JointId, int> jointToBoneIndex;
    JointId rootJoint = JointId::ROOT;

    void BuildSkeleton();
    void BuildDefaultSegments();
    void GenerateSegmentMesh(BodySegment& seg);
    void ComputeForwardKinematics();
    BodySegment MakeCapsule(JointId joint, glm::vec3 offset, float topR, float botR, float height, int sides, int segs);
    BodySegment MakeSphere(JointId joint, glm::vec3 offset, float radius, int latDiv, int lonDiv);
    BodySegment MakeBox(JointId joint, glm::vec3 offset, glm::vec3 dims);
    BodySegment MakeCylinder(JointId joint, glm::vec3 offset, float topR, float botR, float height, int sides, int segs);
    BodySegment MakeCone(JointId joint, glm::vec3 offset, float radius, float height, int sides, int segs);
    void UploadSegment(BodySegment& seg);
    void ComputeNormals(std::vector<ProceduralVertex>& verts, const std::vector<unsigned int>& inds);
    void AddSegment(BodySegment seg);
};
