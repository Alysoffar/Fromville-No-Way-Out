#include "SkeletalAnimator.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace {

glm::mat4 BlendMatrices(const glm::mat4& a, const glm::mat4& b, float t) {
	auto decomposeTRS = [](const glm::mat4& m, glm::vec3& tOut, glm::quat& rOut, glm::vec3& sOut) {
		tOut = glm::vec3(m[3]);

		glm::vec3 col0 = glm::vec3(m[0]);
		glm::vec3 col1 = glm::vec3(m[1]);
		glm::vec3 col2 = glm::vec3(m[2]);

		sOut.x = glm::length(col0);
		sOut.y = glm::length(col1);
		sOut.z = glm::length(col2);

		if (sOut.x > 0.0f) col0 /= sOut.x;
		if (sOut.y > 0.0f) col1 /= sOut.y;
		if (sOut.z > 0.0f) col2 /= sOut.z;

		glm::mat3 rotMat(1.0f);
		rotMat[0] = col0;
		rotMat[1] = col1;
		rotMat[2] = col2;
		rOut = glm::normalize(glm::quat_cast(rotMat));
	};

	glm::vec3 scaleA(1.0f), scaleB(1.0f);
	glm::quat rotA(1.0f, 0.0f, 0.0f, 0.0f), rotB(1.0f, 0.0f, 0.0f, 0.0f);
	glm::vec3 transA(0.0f), transB(0.0f);

	decomposeTRS(a, transA, rotA, scaleA);
	decomposeTRS(b, transB, rotB, scaleB);

	const glm::vec3 blendedT = glm::mix(transA, transB, t);
	const glm::vec3 blendedS = glm::mix(scaleA, scaleB, t);
	const glm::quat blendedR = glm::normalize(glm::slerp(rotA, rotB, t));

	return glm::translate(glm::mat4(1.0f), blendedT) * glm::mat4_cast(blendedR) * glm::scale(glm::mat4(1.0f), blendedS);
}

float WrapAnimationTime(float timeInTicks, const Animation* anim, bool looping) {
	if (!anim) {
		return 0.0f;
	}
	const float duration = std::max(anim->duration, 0.0001f);
	if (looping) {
		return std::fmod(std::max(timeInTicks, 0.0f), duration);
	}
	return std::clamp(timeInTicks, 0.0f, duration);
}

const BoneAnimation* FindBoneAnim(const Animation* anim, const std::string& boneName) {
	if (!anim) {
		return nullptr;
	}
	const auto it = anim->boneAnimationMap.find(boneName);
	if (it == anim->boneAnimationMap.end()) {
		return nullptr;
	}
	return &anim->boneAnimations[it->second];
}

} // namespace

glm::mat4 BoneAnimation::InterpolatePosition(float time) const {
	if (positions.empty()) {
		return glm::mat4(1.0f);
	}
	if (positions.size() == 1U) {
		return glm::translate(glm::mat4(1.0f), positions[0].position);
	}

	const int index = GetPositionIndex(time);
	const int nextIndex = std::min(index + 1, static_cast<int>(positions.size()) - 1);
	const float delta = positions[nextIndex].timeStamp - positions[index].timeStamp;
	const float t = (delta <= 0.0f) ? 0.0f : (time - positions[index].timeStamp) / delta;

	const glm::vec3 blended = glm::mix(positions[index].position, positions[nextIndex].position, std::clamp(t, 0.0f, 1.0f));
	return glm::translate(glm::mat4(1.0f), blended);
}

glm::mat4 BoneAnimation::InterpolateRotation(float time) const {
	if (rotations.empty()) {
		return glm::mat4(1.0f);
	}
	if (rotations.size() == 1U) {
		return glm::mat4_cast(glm::normalize(rotations[0].orientation));
	}

	const int index = GetRotationIndex(time);
	const int nextIndex = std::min(index + 1, static_cast<int>(rotations.size()) - 1);
	const float delta = rotations[nextIndex].timeStamp - rotations[index].timeStamp;
	const float t = (delta <= 0.0f) ? 0.0f : (time - rotations[index].timeStamp) / delta;

	const glm::quat blended = glm::normalize(glm::slerp(rotations[index].orientation, rotations[nextIndex].orientation, std::clamp(t, 0.0f, 1.0f)));
	return glm::mat4_cast(blended);
}

glm::mat4 BoneAnimation::InterpolateScale(float time) const {
	if (scales.empty()) {
		return glm::mat4(1.0f);
	}
	if (scales.size() == 1U) {
		return glm::scale(glm::mat4(1.0f), scales[0].scale);
	}

	const int index = GetScaleIndex(time);
	const int nextIndex = std::min(index + 1, static_cast<int>(scales.size()) - 1);
	const float delta = scales[nextIndex].timeStamp - scales[index].timeStamp;
	const float t = (delta <= 0.0f) ? 0.0f : (time - scales[index].timeStamp) / delta;

	const glm::vec3 blended = glm::mix(scales[index].scale, scales[nextIndex].scale, std::clamp(t, 0.0f, 1.0f));
	return glm::scale(glm::mat4(1.0f), blended);
}

int BoneAnimation::GetPositionIndex(float time) const {
	for (int i = 0; i < static_cast<int>(positions.size()) - 1; ++i) {
		if (time < positions[i + 1].timeStamp) {
			return i;
		}
	}
	return std::max(0, static_cast<int>(positions.size()) - 2);
}

int BoneAnimation::GetRotationIndex(float time) const {
	for (int i = 0; i < static_cast<int>(rotations.size()) - 1; ++i) {
		if (time < rotations[i + 1].timeStamp) {
			return i;
		}
	}
	return std::max(0, static_cast<int>(rotations.size()) - 2);
}

int BoneAnimation::GetScaleIndex(float time) const {
	for (int i = 0; i < static_cast<int>(scales.size()) - 1; ++i) {
		if (time < scales[i + 1].timeStamp) {
			return i;
		}
	}
	return std::max(0, static_cast<int>(scales.size()) - 2);
}

SkeletalAnimator::SkeletalAnimator(Model* model)
	: model(model), finalBoneMatrices(100, glm::mat4(1.0f)) {
	if (!model) {
		throw std::runtime_error("SkeletalAnimator requires a valid Model pointer.");
	}

	glGenBuffers(1, &boneUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, boneUBO);
	glBufferData(GL_UNIFORM_BUFFER, static_cast<GLsizeiptr>(100 * sizeof(glm::mat4)), nullptr, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, boneUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void SkeletalAnimator::LoadAnimation(const std::string& name, int animationIndex) {
	if (!model || !model->scene) {
		throw std::runtime_error("Cannot load animation without a loaded model scene.");
	}

	Animation animation = LoadAnimationFromAssimp(model->scene, animationIndex);
	animation.name = name;
	animations[name] = std::move(animation);
}

void SkeletalAnimator::PlayAnimation(const std::string& name, bool loop) {
	auto it = animations.find(name);
	if (it == animations.end()) {
		throw std::runtime_error("Animation not loaded: " + name);
	}

	currentAnim = &it->second;
	currentTime = 0.0f;
	looping = loop;
	blendTargetAnim = nullptr;
	blendTime = 0.0f;
	blendDuration = 0.0f;
}

void SkeletalAnimator::BlendTo(const std::string& name, float duration) {
	auto it = animations.find(name);
	if (it == animations.end()) {
		throw std::runtime_error("Blend target animation not loaded: " + name);
	}

	if (!currentAnim) {
		currentAnim = &it->second;
		currentTime = 0.0f;
		blendTargetAnim = nullptr;
		return;
	}

	blendTargetAnim = &it->second;
	blendTime = 0.0f;
	blendDuration = std::max(duration, 0.0001f);
}

void SkeletalAnimator::Update(float deltaTime) {
	if (!currentAnim) {
		return;
	}

	const float ticksPerSecondA = (currentAnim->ticksPerSecond <= 0.0f) ? 25.0f : currentAnim->ticksPerSecond;
	currentTime += deltaTime * ticksPerSecondA;

	if (blendTargetAnim) {
		blendTime += deltaTime;
		const float blend = std::clamp(blendTime / blendDuration, 0.0f, 1.0f);

		const float timeA = WrapAnimationTime(currentTime, currentAnim, looping);
		const float ticksPerSecondB = (blendTargetAnim->ticksPerSecond <= 0.0f) ? 25.0f : blendTargetAnim->ticksPerSecond;
		const float rawB = currentTime * (ticksPerSecondB / ticksPerSecondA);
		const float timeB = WrapAnimationTime(rawB, blendTargetAnim, true);

		BlendBoneTransform(&currentAnim->rootNode, glm::mat4(1.0f), timeA, timeB, blend);

		if (blend >= 1.0f) {
			currentAnim = blendTargetAnim;
			blendTargetAnim = nullptr;
			currentTime = timeB;
			blendTime = 0.0f;
			blendDuration = 0.0f;
		}
	} else {
		const float animTime = WrapAnimationTime(currentTime, currentAnim, looping);
		CalculateBoneTransform(&currentAnim->rootNode, glm::mat4(1.0f), animTime, currentAnim);
	}
}

void SkeletalAnimator::UploadToUBO() {
	glBindBuffer(GL_UNIFORM_BUFFER, boneUBO);
	glBufferSubData(
		GL_UNIFORM_BUFFER,
		0,
		static_cast<GLsizeiptr>(finalBoneMatrices.size() * sizeof(glm::mat4)),
		finalBoneMatrices.data());
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

bool SkeletalAnimator::IsPlaying(const std::string& name) const {
	return currentAnim && currentAnim->name == name;
}

void SkeletalAnimator::CalculateBoneTransform(const AssimpNodeData* node, glm::mat4 parentTransform, float time, Animation* anim) {
	if (!node || !anim || !model) {
		return;
	}

	glm::mat4 nodeTransform = node->transformation;
	const BoneAnimation* boneAnim = FindBoneAnim(anim, node->name);
	if (boneAnim) {
		nodeTransform = boneAnim->InterpolatePosition(time) * boneAnim->InterpolateRotation(time) * boneAnim->InterpolateScale(time);
	}

	const glm::mat4 globalTransform = parentTransform * nodeTransform;

	const auto boneIt = model->boneInfoMap.find(node->name);
	if (boneIt != model->boneInfoMap.end()) {
		const int boneId = boneIt->second.id;
		if (boneId >= 0 && boneId < static_cast<int>(finalBoneMatrices.size())) {
			finalBoneMatrices[boneId] = model->globalInverseTransform * globalTransform * boneIt->second.offsetMatrix;
		}
	}

	for (const AssimpNodeData& child : node->children) {
		CalculateBoneTransform(&child, globalTransform, time, anim);
	}
}

glm::mat4 SkeletalAnimator::BlendBoneTransform(const AssimpNodeData* node, glm::mat4 parentTransform, float timeA, float timeB, float blend) {
	if (!node || !currentAnim || !blendTargetAnim || !model) {
		return parentTransform;
	}

	glm::mat4 localA = node->transformation;
	glm::mat4 localB = node->transformation;

	if (const BoneAnimation* boneA = FindBoneAnim(currentAnim, node->name)) {
		localA = boneA->InterpolatePosition(timeA) * boneA->InterpolateRotation(timeA) * boneA->InterpolateScale(timeA);
	}
	if (const BoneAnimation* boneB = FindBoneAnim(blendTargetAnim, node->name)) {
		localB = boneB->InterpolatePosition(timeB) * boneB->InterpolateRotation(timeB) * boneB->InterpolateScale(timeB);
	}

	const glm::mat4 blendedLocal = BlendMatrices(localA, localB, std::clamp(blend, 0.0f, 1.0f));
	const glm::mat4 globalTransform = parentTransform * blendedLocal;

	const auto boneIt = model->boneInfoMap.find(node->name);
	if (boneIt != model->boneInfoMap.end()) {
		const int boneId = boneIt->second.id;
		if (boneId >= 0 && boneId < static_cast<int>(finalBoneMatrices.size())) {
			finalBoneMatrices[boneId] = model->globalInverseTransform * globalTransform * boneIt->second.offsetMatrix;
		}
	}

	for (const AssimpNodeData& child : node->children) {
		BlendBoneTransform(&child, globalTransform, timeA, timeB, blend);
	}

	return globalTransform;
}

Animation SkeletalAnimator::LoadAnimationFromAssimp(const aiScene* scene, int index) {
	if (!scene || scene->mNumAnimations == 0) {
		throw std::runtime_error("No animations available in assimp scene.");
	}
	if (index < 0 || index >= static_cast<int>(scene->mNumAnimations)) {
		throw std::runtime_error("Animation index out of range.");
	}

	const aiAnimation* aiAnim = scene->mAnimations[index];
	if (!aiAnim) {
		throw std::runtime_error("Assimp animation pointer is null.");
	}

	Animation out;
	out.name = aiAnim->mName.C_Str();
	out.duration = static_cast<float>(aiAnim->mDuration);
	out.ticksPerSecond = (aiAnim->mTicksPerSecond > 0.0) ? static_cast<float>(aiAnim->mTicksPerSecond) : 25.0f;

	out.boneAnimations.reserve(aiAnim->mNumChannels);
	for (unsigned int i = 0; i < aiAnim->mNumChannels; ++i) {
		const aiNodeAnim* channel = aiAnim->mChannels[i];
		if (!channel) {
			continue;
		}

		BoneAnimation boneAnim;
		boneAnim.name = channel->mNodeName.C_Str();

		const auto it = model->boneInfoMap.find(boneAnim.name);
		boneAnim.id = (it != model->boneInfoMap.end()) ? it->second.id : -1;

		boneAnim.positions.reserve(channel->mNumPositionKeys);
		for (unsigned int p = 0; p < channel->mNumPositionKeys; ++p) {
			boneAnim.positions.push_back({AssimpToGlm(channel->mPositionKeys[p].mValue), static_cast<float>(channel->mPositionKeys[p].mTime)});
		}

		boneAnim.rotations.reserve(channel->mNumRotationKeys);
		for (unsigned int r = 0; r < channel->mNumRotationKeys; ++r) {
			boneAnim.rotations.push_back({AssimpToGlm(channel->mRotationKeys[r].mValue), static_cast<float>(channel->mRotationKeys[r].mTime)});
		}

		boneAnim.scales.reserve(channel->mNumScalingKeys);
		for (unsigned int s = 0; s < channel->mNumScalingKeys; ++s) {
			boneAnim.scales.push_back({AssimpToGlm(channel->mScalingKeys[s].mValue), static_cast<float>(channel->mScalingKeys[s].mTime)});
		}

		out.boneAnimationMap[boneAnim.name] = static_cast<int>(out.boneAnimations.size());
		out.boneAnimations.push_back(std::move(boneAnim));
	}

	CopyNodeHierarchy(out.rootNode, scene->mRootNode);
	return out;
}

void SkeletalAnimator::CopyNodeHierarchy(AssimpNodeData& dest, const aiNode* src) {
	if (!src) {
		return;
	}

	dest.name = src->mName.C_Str();
	dest.transformation = AssimpToGlm(src->mTransformation);
	dest.children.clear();
	dest.children.reserve(src->mNumChildren);

	for (unsigned int i = 0; i < src->mNumChildren; ++i) {
		AssimpNodeData child;
		CopyNodeHierarchy(child, src->mChildren[i]);
		dest.children.push_back(std::move(child));
	}
}

glm::mat4 SkeletalAnimator::AssimpToGlm(const aiMatrix4x4& m) {
	glm::mat4 out(1.0f);
	out[0][0] = m.a1; out[0][1] = m.a2; out[0][2] = m.a3; out[0][3] = m.a4;
	out[1][0] = m.b1; out[1][1] = m.b2; out[1][2] = m.b3; out[1][3] = m.b4;
	out[2][0] = m.c1; out[2][1] = m.c2; out[2][2] = m.c3; out[2][3] = m.c4;
	out[3][0] = m.d1; out[3][1] = m.d2; out[3][2] = m.d3; out[3][3] = m.d4;
	return out;
}

glm::quat SkeletalAnimator::AssimpToGlm(const aiQuaternion& q) {
	return glm::quat(q.w, q.x, q.y, q.z);
}

glm::vec3 SkeletalAnimator::AssimpToGlm(const aiVector3D& v) {
	return glm::vec3(v.x, v.y, v.z);
}

