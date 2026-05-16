#version 330 core
out vec4 FragColor;

in vec3 vFragPos;
in vec3 vNormal;
in vec2 vTexCoord;

uniform sampler2D uAlbedoMap;
uniform sampler2D uNormalMap;
uniform sampler2D uRoughnessMap;

uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uAmbient;
uniform float uDiffuseStrength;
uniform vec3 uViewPos;
uniform vec3 uFogColor;
uniform float uFogDensity;

void main() {
    vec3 albedo = texture(uAlbedoMap, vTexCoord).rgb;
    vec3 normalMap = texture(uNormalMap, vTexCoord).rgb;
    float roughness = texture(uRoughnessMap, vTexCoord).r;

    // TBN-lite for flat ground
    vec3 N = normalize(vNormal);
    vec3 T = normalize(vec3(1.0, 0.0, 0.0));
    vec3 B = cross(N, T);
    mat3 TBN = mat3(T, B, N);

    vec3 normal = normalize(TBN * (normalMap * 2.0 - 1.0));
    vec3 lightDir = normalize(-uLightDir);
    vec3 viewDir = normalize(uViewPos - vFragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);

    // Blinn-Phong
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * uLightColor * uDiffuseStrength * albedo;
    
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    vec3 specular = spec * uLightColor * pow(1.0 - roughness, 4.0);

    vec3 litColor = (uAmbient * albedo) + diffuse + specular;

    float dist = length(uViewPos - vFragPos);
    float fogFactor = exp(-uFogDensity * dist * dist);
    vec3 finalColor = mix(uFogColor, litColor, clamp(fogFactor, 0.0, 1.0));

    FragColor = vec4(finalColor, 1.0);
}
