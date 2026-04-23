#version 450 core

layout(location = 0) out vec3 gPosition;
layout(location = 1) out vec3 gNormal;
layout(location = 2) out vec4 gAlbedoSpec;
layout(location = 3) out vec3 gEmission;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in mat3 TBN;

uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D roughnessMap;
uniform sampler2D metallicMap;
uniform sampler2D aoMap;
uniform sampler2D emissiveMap;

uniform bool hasNormalMap;
uniform bool hasEmissiveMap;

void main() {
	vec3 normal = normalize(Normal);
	if (hasNormalMap) {
		vec3 tangentNormal = texture(normalMap, TexCoords).xyz * 2.0 - 1.0;
		normal = normalize(TBN * tangentNormal);
	}

	vec4 albedo = texture(albedoMap, TexCoords);
	float roughness = texture(roughnessMap, TexCoords).r;
	vec3 emissive = vec3(0.0);
	if (hasEmissiveMap) {
		emissive = texture(emissiveMap, TexCoords).rgb * 2.0;
	}

	gPosition = FragPos;
	gNormal = normal;
	gAlbedoSpec = vec4(albedo.rgb, roughness);
	gEmission = emissive;
}
