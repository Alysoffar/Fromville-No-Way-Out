#version 450 core
out vec4 fragColor;

in vec2 TexCoords;

layout(binding = 0) uniform sampler2D gPosition;
layout(binding = 1) uniform sampler2D gNormal;
layout(binding = 2) uniform sampler2D gAlbedoSpec;
layout(binding = 3) uniform sampler2D gEmission;
layout(binding = 4) uniform sampler2D ssaoBlur;
layout(binding = 5) uniform sampler2D shadowMap;

uniform mat4 lightSpaceMatrix;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform vec3 ambientColor;
uniform vec3 fogColor;
uniform float fogDensity;
uniform float time;

const float PI = 3.14159265359;

vec3 SkyColor(vec2 uv, float timeOfDay) {
    vec3 zenith;
    vec3 horizon;
    vec3 glowColor = vec3(0.10, 0.12, 0.18);

    if (timeOfDay >= 0.2 && timeOfDay < 0.3) {
        float t = clamp((timeOfDay - 0.2) / 0.1, 0.0, 1.0);
        zenith = mix(vec3(0.04, 0.03, 0.13), vec3(0.16, 0.34, 0.68), t);
        horizon = mix(vec3(0.83, 0.45, 0.04), vec3(0.53, 0.81, 0.92), t);
        glowColor = mix(vec3(0.92, 0.48, 0.12), vec3(0.95, 0.78, 0.42), t);
    } else if (timeOfDay >= 0.3 && timeOfDay < 0.7) {
        float t = smoothstep(0.3, 0.55, timeOfDay) * (1.0 - smoothstep(0.58, 0.7, timeOfDay));
        zenith = mix(vec3(0.16, 0.34, 0.72), vec3(0.10, 0.26, 0.58), t);
        horizon = mix(vec3(0.62, 0.80, 0.96), vec3(0.78, 0.88, 0.98), t);
        glowColor = vec3(0.95, 0.95, 0.90);
    } else if (timeOfDay >= 0.7 && timeOfDay < 0.8) {
        float t = clamp((timeOfDay - 0.7) / 0.1, 0.0, 1.0);
        zenith = mix(vec3(0.12, 0.10, 0.25), vec3(0.03, 0.03, 0.10), t);
        horizon = mix(vec3(0.85, 0.45, 0.15), vec3(0.18, 0.12, 0.18), t);
        glowColor = mix(vec3(0.86, 0.38, 0.14), vec3(0.16, 0.18, 0.24), t);
    } else {
        zenith = vec3(0.01, 0.01, 0.06);
        horizon = vec3(0.03, 0.04, 0.08);
        glowColor = vec3(0.12, 0.16, 0.28);
    }

    float horizonMix = smoothstep(0.0, 0.55, uv.y);
    vec3 sky = mix(horizon, zenith, horizonMix);

    float cloudBand = smoothstep(0.12, 0.82, uv.y);
    float cloudNoise = 0.5 + 0.5 * sin(uv.x * 14.0 + uv.y * 10.0 + timeOfDay * 4.0);
    float cloudShape = smoothstep(0.40, 0.68, cloudNoise) * cloudBand;
    sky += vec3(0.08, 0.09, 0.10) * cloudShape * 0.14;
    sky += vec3(0.10, 0.12, 0.15) * smoothstep(0.0, 0.22, 1.0 - uv.y) * 0.24;
    sky += vec3(0.02, 0.03, 0.05) * smoothstep(0.60, 0.0, abs(uv.x - 0.5)) * smoothstep(0.03, 0.25, uv.y) * 0.08;

    float orbit = fract(timeOfDay + 0.75);
    vec2 sunCenter = vec2(0.65 + sin(orbit * 6.28318) * 0.18, 0.63 + cos(orbit * 6.28318) * 0.18);
    float glow = exp(-24.0 * distance(uv, sunCenter));
    sky += glowColor * glow * 1.1;

    return sky;
}

float DirectionalShadowPCF(vec3 fragPos, vec3 N, vec3 L) {
    vec4 fragPosLightSpace = lightSpaceMatrix * vec4(fragPos, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / max(fragPosLightSpace.w, 0.0001);
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 0.0;
    }

    float currentDepth = projCoords.z;
    float bias = max(0.005 * (1.0 - dot(N, L)), 0.0005);
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));

    float shadow = 0.0;
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }

    return shadow / 9.0;
}

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

vec3 ApplyToneMap(vec3 color) {
    color = max(color, vec3(0.0));
    color = color / (color + vec3(1.0));
    return pow(color, vec3(1.0 / 2.2));
}

void main()
{
    vec3 fragPos = texture(gPosition, TexCoords).xyz;
    vec3 normal = texture(gNormal, TexCoords).xyz;
    vec4 albedoSpec = texture(gAlbedoSpec, TexCoords);
    vec3 albedo = pow(albedoSpec.rgb, vec3(2.2));
    vec3 emission = texture(gEmission, TexCoords).rgb;
    float ao = clamp(texture(ssaoBlur, TexCoords).r, 0.0, 1.0);

    if(length(normal) < 0.1) {
        fragColor = vec4(SkyColor(TexCoords, time) + emission, 1.0);
        return;
    }

    float roughness = clamp(albedoSpec.a, 0.04, 1.0);
    float metallic = 0.0;
    
    vec3 N = normalize(normal);
    vec3 V = normalize(viewPos - fragPos);

    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    vec3 Lo = vec3(0.0);

    vec3 L = normalize(-lightDir);
    vec3 H = normalize(V + L);

    float shadow = DirectionalShadowPCF(fragPos, N, L);

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

    vec3 skyLight = mix(fogColor, vec3(1.0, 0.98, 0.94), clamp(N.y * 0.5 + 0.5, 0.0, 1.0));
    float horizonBoost = smoothstep(-0.20, 0.40, N.y);
    vec3 bouncedLight = skyLight * ambientColor * (0.28 + 0.22 * horizonBoost);

    vec3 ambient = ambientColor * albedo * ao + bouncedLight * albedo;

    vec3 color = ambient + Lo + emission;

    float dist = length(viewPos - fragPos);
    float fogFactor = exp(-pow(fogDensity * dist, 2.0));
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    color = mix(fogColor, color, fogFactor);

    color = ApplyToneMap(color * 1.06);

    fragColor = vec4(color, 1.0);
}
