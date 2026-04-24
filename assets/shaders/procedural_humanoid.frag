#version 450 core

layout(location = 0) out vec3 gPosition;
layout(location = 1) out vec3 gNormal;
layout(location = 2) out vec4 gAlbedoSpec;
layout(location = 3) out vec3 gEmission;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 albedoColor;
uniform float roughness;
uniform float metalness;
uniform vec3 emission;

void main() {
    vec3 n = normalize(Normal);
    float aoShade = mix(0.92, 1.0, clamp(TexCoords.y, 0.0, 1.0));

    gPosition = FragPos;
    gNormal = n;
    gAlbedoSpec = vec4(albedoColor * aoShade, clamp(roughness + metalness * 0.05, 0.05, 1.0));
    gEmission = emission;
}
