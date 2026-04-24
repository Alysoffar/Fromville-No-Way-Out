#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <assimp/scene.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Model.h"

struct KeyPosition  { glm::vec3 position; float timeStamp; };
struct KeyRotation  { glm::quat orientation; float timeStamp; };
struct KeyScale     { glm::vec3 scale; float timeStamp; };

struct BoneAnimation {
	std::string name;
	int id = -1;
	std::vector<KeyPosition> positions;
	std::vector<KeyRotation> rotations;
	std::vector<KeyScale> scales;

	glm::mat4 InterpolatePosition(float time) const;
	glm::mat4 InterpolateRotation(float time) const;
	glm::mat4 InterpolateScale(float time) const;
	int GetPositionIndex(float time) const;
	int GetRotationIndex(float time) const;
	int GetScaleIndex(float time) const;
};

struct AssimpNodeData {
	glm::mat4 transformation = glm::mat4(1.0f);
	std::string name;
	std::vector<AssimpNodeData> children;
};

struct Animation {
	std::string name;
	float duration = 0.0f;
	float ticksPerSecond = 25.0f;
	std::vector<BoneAnimation> boneAnimations;
	std::unordered_map<std::string, int> boneAnimationMap;
	AssimpNodeData rootNode;
};

class SkeletalAnimator {
public:
	explicit SkeletalAnimator(Model* model);
	void LoadAnimation(const std::string& name, int animationIndex = 0);
	void LoadAnimationFromFile(const std::string& name, const std::string& path, int animationIndex = 0);
	void PlayAnimation(const std::string& name, bool loop = true);
	void BlendTo(const std::string& name, float blendDuration);
	void Update(float deltaTime);
	const std::vector<glm::mat4>& GetBoneMatrices() const { return finalBoneMatrices; }
	void UploadToUBO();
	bool IsPlaying(const std::string& name) const;
	float GetAnimationDuration(const std::string& name) const;

private:
	Model* model;
	std::unordered_map<std::string, Animation> animations;
	Animation* currentAnim = nullptr;
	Animation* blendTargetAnim = nullptr;
	float currentTime = 0.0f;
	float blendTime = 0.0f;
	float blendDuration = 0.0f;
	bool looping = true;
	std::vector<glm::mat4> finalBoneMatrices;
	GLuint boneUBO = 0;

	void CalculateBoneTransform(const AssimpNodeData* node, glm::mat4 parentTransform, float time, Animation* anim);
	void CalculateBoneTransform(const AssimpNodeData* node, glm::mat4 parentTransform, float timeA, float timeB, float blend);
	Animation LoadAnimationFromAssimp(const aiScene* scene, int index);
	void CopyNodeHierarchy(AssimpNodeData& dest, const aiNode* src);
	glm::mat4 AssimpToGlm(const aiMatrix4x4& m);
	glm::quat AssimpToGlm(const aiQuaternion& q);
	glm::vec3 AssimpToGlm(const aiVector3D& v);
};
