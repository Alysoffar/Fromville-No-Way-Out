#pragma once

#include <glm/glm.hpp>
#include <map>
#include <vector>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include "Animation.h"
#include "Bone.h"

class Animator {
public:
    Animator(Animation* animation, bool applyGlobalInverse = false) {
        m_CurrentTime = 0.0;
        m_CurrentAnimation = animation;
        m_ApplyGlobalInverse = applyGlobalInverse;

        m_FinalBoneMatrices.reserve(100);
        for (int i = 0; i < 100; i++)
            m_FinalBoneMatrices.push_back(glm::mat4(1.0f));
    }

    void SetAnimation(Animation* animation) {
        m_CurrentAnimation = animation;
        if (!m_CurrentAnimation || m_CurrentAnimation->GetDuration() <= 0.0f) {
            for (int i = 0; i < 100; i++) {
                m_FinalBoneMatrices[i] = glm::mat4(1.0f);
            }
        }
    }

    void UpdateAnimation(float dt) {
        m_DeltaTime = dt;
        if (m_CurrentAnimation && m_CurrentAnimation->GetDuration() > 0.0f) {
            m_CurrentTime += m_CurrentAnimation->GetTicksPerSecond() * dt;
            m_CurrentTime = fmod(m_CurrentTime, m_CurrentAnimation->GetDuration());
            CalculateBoneTransform(&m_CurrentAnimation->GetRootNode(), glm::mat4(1.0f));
        } else {
            for (int i = 0; i < 100; i++) {
                m_FinalBoneMatrices[i] = glm::mat4(1.0f);
            }
        }
    }

    void PlayAnimation(Animation* pAnimation) {
        SetAnimation(pAnimation);
        m_CurrentTime = 0.0f;
    }

    void CalculateBoneTransform(const AssimpNodeData* node, glm::mat4 parentTransform) {
        std::string nodeName = node->name;
        glm::mat4 nodeTransform = node->transformation;

        Bone* Bone = m_CurrentAnimation->FindBone(nodeName);

        if (Bone) {
            Bone->Update(m_CurrentTime);
            nodeTransform = Bone->GetLocalTransform();
        }

        glm::mat4 globalTransformation = parentTransform * nodeTransform;

        auto boneInfoMap = m_CurrentAnimation->GetBoneIDMap();
        if (boneInfoMap.find(nodeName) != boneInfoMap.end()) {
            int index = boneInfoMap[nodeName].id;
            glm::mat4 offset = boneInfoMap[nodeName].offset;
            glm::mat4 finalMatrix = globalTransformation * offset;
            if (m_ApplyGlobalInverse) {
                finalMatrix = m_CurrentAnimation->GetGlobalInverseTransform() * finalMatrix;
            }
            m_FinalBoneMatrices[index] = finalMatrix;
        }

        for (int i = 0; i < node->childrenCount; i++)
            CalculateBoneTransform(&node->children[i], globalTransformation);
    }

    Animation* GetCurrentAnimation() const { return m_CurrentAnimation; }

    const std::vector<glm::mat4>& GetFinalBoneMatrices() const {
        if (!m_CurrentAnimation || m_CurrentAnimation->GetDuration() <= 0.0f) {
            static const std::vector<glm::mat4> identity(100, glm::mat4(1.0f));
            return identity;
        }
        return m_FinalBoneMatrices;
    }

private:
    std::vector<glm::mat4> m_FinalBoneMatrices;
    Animation* m_CurrentAnimation;
    float m_CurrentTime;
    float m_DeltaTime;
    bool m_ApplyGlobalInverse = false;
};
