#pragma once

#include "Bone.h"
#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glad/glad.h>
#include <vector>
#include <string>
#include <map>
#include <memory>

struct AssimpNodeData {
    glm::mat4 transformation;
    std::string name;
    int childrenCount;
    std::vector<AssimpNodeData> children;
};

class AnimatedMesh {
public:
    explicit AnimatedMesh(const std::string& path);
    ~AnimatedMesh();

    void draw(GLuint shaderProgram) const;
    const std::map<std::string, BoneInfo>& getBoneInfoMap() const { return m_BoneInfoMap; }
    int getBoneCount() const { return m_BoneCounter; }
    const AssimpNodeData& getRootNode() const { return m_RootNode; }
    const glm::mat4& getGlobalInverseTransform() const { return m_GlobalInverseTransform; }
    bool IsValid() const { return m_VAO != 0 && !m_Indices.empty(); }

private:
    std::vector<Vertex>    m_Vertices;
    std::vector<uint32_t>  m_Indices;
    std::map<std::string, BoneInfo> m_BoneInfoMap;
    int m_BoneCounter = 0;
    GLuint m_VAO, m_VBO, m_EBO;
    GLuint m_TextureID = 0;
    bool m_TextureLoaded = false;

    AssimpNodeData m_RootNode;
    glm::mat4 m_GlobalInverseTransform;

    void loadModel(const std::string& path);
    void processNode(aiNode* node, const aiScene* scene);
    void processMesh(aiMesh* mesh, const aiScene* scene);
    void extractBoneWeights(aiMesh* mesh);
    void setupMesh();
    void ReadHierarchyData(AssimpNodeData& dest, const aiNode* src);

    static glm::mat4 ConvertMatrixToGLMFormat(const aiMatrix4x4& from);
};
