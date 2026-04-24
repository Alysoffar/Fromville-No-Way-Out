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
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 ambientColor;

void main() {
    vec3 base = texture(albedoMap, TexCoords).rgb;
    if (base == vec3(0.0)) {
        base = vec3(1.0); // Use white if no texture, multiply by tint later
    }

    vec3 n = normalize(Normal);
    float ndl = max(dot(n, lightDir), 0.0);

    vec3 diffuse = base * tintColor * (ambientColor + lightColor * ndl);
    vec3 color = diffuse;
    if (hasEmissiveMap) {
        color += texture(emissiveMap, TexCoords).rgb * 0.5;
    }

    FragColor = vec4(color, 1.0);
}
