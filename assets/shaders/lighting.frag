#version 450 core
out vec4 fragColor;

in vec2 TexCoords;

layout(binding = 0) uniform sampler2D gPosition;
layout(binding = 1) uniform sampler2D gNormal;
layout(binding = 2) uniform sampler2D gAlbedoSpec;
layout(binding = 3) uniform sampler2D gEmission;
layout(binding = 4) uniform sampler2D ssaoBlur;
layout(binding = 5) uniform sampler2D shadowMap;
layout(binding = 6) uniform samplerCube shadowCubemaps[8];

struct PointLight {
    vec3 position;
    vec3 color;
    float radius;
    float intensity;
};

layout(std430, binding=1) buffer LightBuffer {
    int lightCount;
    PointLight lights[];
};

uniform mat4 lightSpaceMatrix;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform float farPlane;
uniform vec2 screenSize;
uniform float ambientStrength;
uniform vec3 fogColor;
uniform float fogDensity;
uniform float time;

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
    
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return num / max(denom, 0.0000001);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;
    
    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main()
{
    // 1. Sample G-Buffer
    vec3 fragPos = texture(gPosition, TexCoords).xyz;
    vec3 normal = texture(gNormal, TexCoords).xyz;
    vec4 albedoSpec = texture(gAlbedoSpec, TexCoords);
    vec3 albedo = albedoSpec.rgb;
    vec3 emission = texture(gEmission, TexCoords).rgb;
    float ssao = texture(ssaoBlur, TexCoords).r;

    // Background mask check
    if(length(normal) < 0.1) {
        fragColor = vec4(albedo + emission, 1.0);
        return;
    }

    // 2. Reconstruct material properties
    float roughness = albedoSpec.a;
    float metallic = 0.0; // No metallic map yet
    
    vec3 N = normalize(normal);
    vec3 V = normalize(viewPos - fragPos);

    // 3. F0
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    vec3 Lo = vec3(0.0);

    // 4. Directional Light (Moon)
    vec3 L = normalize(lightDir);
    vec3 H = normalize(V + L);
    
    // PCF shadow
    vec4 fragPosLightSpace = lightSpaceMatrix * vec4(fragPos, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    float shadow = 0.0;
    if(projCoords.z <= 1.0) {
        float currentDepth = projCoords.z;
        float bias = max(0.005 * (1.0 - dot(N, L)), 0.0005);
        vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
        for(int x = -1; x <= 1; ++x) {
            for(int y = -1; y <= 1; ++y) {
                float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
                shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
            }    
        }
        shadow /= 9.0;
    }
    
    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;
    
    float NdotL = max(dot(N, L), 0.0);
    Lo += (kD * albedo / PI + specular) * lightColor * NdotL * (1.0 - shadow);

    // 5. Point Lights
    int count = min(lightCount, 32);
    for(int i = 0; i < count; i++) {
        vec3 pL = normalize(lights[i].position - fragPos);
        vec3 pH = normalize(V + pL);
        float dist = length(lights[i].position - fragPos);
        
        // Attenuation
        float distDiv = clamp(1.0 - pow(dist / lights[i].radius, 2.0), 0.0, 1.0);
        float attenuation = distDiv * distDiv * lights[i].intensity;
        
        float pointShadow = 0.0;
        if(i < 8) {
            vec3 fragToLight = fragPos - lights[i].position;
            float closestDepth = texture(shadowCubemaps[i], fragToLight).r;
            closestDepth *= farPlane; // Undo [0,1] mapping
            float pointBias = 0.05; 
            pointShadow = (dist - pointBias > closestDepth) ? 1.0 : 0.0;
        }

        // Cook-Torrance
        float pNDF = DistributionGGX(N, pH, roughness);
        float pG = GeometrySmith(N, V, pL, roughness);
        vec3 pF = FresnelSchlick(max(dot(pH, V), 0.0), F0);
        
        vec3 pkS = pF;
        vec3 pkD = vec3(1.0) - pkS;
        pkD *= 1.0 - metallic;
        
        vec3 pNumerator = pNDF * pG * pF;
        float pDenominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, pL), 0.0) + 0.0001;
        vec3 pSpecular = pNumerator / pDenominator;
        
        float pNdotL = max(dot(N, pL), 0.0);
        Lo += (pkD * albedo / PI + pSpecular) * lights[i].color * pNdotL * attenuation * (1.0 - pointShadow);
    }

    // 6. Ambient
    vec3 ambient = ambientStrength * albedo * ssao;

    // 7. Combine + Emission
    vec3 color = ambient + Lo + emission;

    // 8. Exponential-squared fog
    float viewDist = length(viewPos - fragPos);
    float fogFactor = exp(-pow(fogDensity * viewDist, 2.0));
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    color = mix(fogColor, color, fogFactor);

    fragColor = vec4(color, 1.0);
}
