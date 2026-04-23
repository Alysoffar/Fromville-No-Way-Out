#version 450 core
out vec4 FragColor;

in vec3 Normal;
in vec2 TexCoords;
in vec3 WorldPos;

uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D roughnessMap;
uniform sampler2D metallicMap;
uniform sampler2D aoMap;
uniform sampler2D emissiveMap;
uniform bool hasNormalMap;
uniform bool hasEmissiveMap;
uniform vec3 tintColor;

void main() {
    vec3 base = texture(albedoMap, TexCoords).rgb;
    if (base == vec3(0.0)) {
        base = tintColor;
    }

    vec3 n = normalize(Normal);
    vec3 lightDir = normalize(vec3(-0.35, 0.8, -0.25));
    float ndl = max(dot(n, lightDir), 0.15);

    vec3 color = base * tintColor * ndl;
    if (hasEmissiveMap) {
        color += texture(emissiveMap, TexCoords).rgb * 0.5;
    }

    FragColor = vec4(color, 1.0);
}
