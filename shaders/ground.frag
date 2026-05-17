#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

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

void main()
{
    // Sample textures
    vec3 albedo = texture(uAlbedoMap, TexCoords).rgb;
    vec3 normalMapSample = texture(uNormalMap, TexCoords).rgb;
    float roughness = texture(uRoughnessMap, TexCoords).r;

    // Reconstruct TBN from world normal (flat ground, normal = 0,1,0)
    vec3 N = normalize(Normal);
    vec3 T = normalize(vec3(1.0, 0.0, 0.0)); // tangent along X for flat ground
    vec3 B = cross(N, T);
    mat3 TBN = mat3(T, B, N);

    // Apply normal map: convert from [0,1] to [-1,1]
    vec3 tangentNormal = normalMapSample * 2.0 - 1.0;
    vec3 perturbedNormal = normalize(TBN * tangentNormal);

    // Lighting
    vec3 lightDir = normalize(-uLightDir);

    // Ambient — driven by day/night cycle uniform
    vec3 ambient = uAmbient * albedo;

    // Diffuse — strength driven by day/night cycle uniform
    float diff = max(dot(perturbedNormal, lightDir), 0.0);
    vec3 diffuse = diff * uLightColor * uDiffuseStrength * albedo;

    // Specular (Blinn-Phong, suppressed by roughness)
    vec3 viewDir = normalize(uViewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(perturbedNormal, halfwayDir), 0.0), 32.0);
    // Suppress specular when roughness is high
    float specularStrength = max(0.0, 1.0 - roughness) * 0.3;
    if (roughness > 0.7) {
        specularStrength *= 0.05; // near zero
    }
    vec3 specular = specularStrength * spec * uLightColor;

    vec3 litColor = ambient + diffuse + specular;

    // Distance fog
    float fragDistance = length(FragPos - uViewPos);
    float fogFactor = exp(-uFogDensity * fragDistance * fragDistance * 0.5);
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    vec3 finalColor = mix(uFogColor, litColor, fogFactor);

    FragColor = vec4(finalColor, 1.0);
}
