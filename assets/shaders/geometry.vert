#version 450 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;
layout(location = 5) in ivec4 aBoneIDs;
layout(location = 6) in vec4 aBoneWeights;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

layout(std140, binding = 0) uniform BoneMatrices {
	mat4 bones[100];
};

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out mat3 TBN;

void main() {
	mat4 skinMatrix = mat4(0.0);
	bool hasWeights = false;
	for (int i = 0; i < 4; ++i) {
		const float weight = aBoneWeights[i];
		if (weight > 0.0) {
			hasWeights = true;
			skinMatrix += bones[aBoneIDs[i]] * weight;
		}
	}
	if (!hasWeights) {
		skinMatrix = mat4(1.0);
	}

	vec4 skinnedPos = skinMatrix * vec4(aPos, 1.0);
	vec3 skinnedNormal = mat3(skinMatrix) * aNormal;
	vec3 skinnedTangent = mat3(skinMatrix) * aTangent;
	vec3 skinnedBitangent = mat3(skinMatrix) * aBitangent;

	vec4 worldPos = model * skinnedPos;
	mat3 normalMatrix = mat3(transpose(inverse(model)));
	vec3 worldNormal = normalize(normalMatrix * skinnedNormal);
	vec3 worldTangent = normalize(normalMatrix * skinnedTangent);
	vec3 worldBitangent = normalize(normalMatrix * skinnedBitangent);

	FragPos = worldPos.xyz;
	Normal = worldNormal;
	TexCoords = aTexCoords;
	TBN = mat3(worldTangent, worldBitangent, worldNormal);
	gl_Position = projection * view * worldPos;
}
