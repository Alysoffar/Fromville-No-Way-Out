#version 450 core
out float fragColor;

in vec2 TexCoords;

layout(binding = 0) uniform sampler2D gPosition;
layout(binding = 1) uniform sampler2D gNormal;
layout(binding = 2) uniform sampler2D noiseTexture;

uniform vec3 samples[64];
uniform mat4 projection;
uniform mat4 view;
uniform vec2 noiseScale;

void main()
{
    vec3 fragPos = (view * vec4(texture(gPosition, TexCoords).xyz, 1.0)).xyz;
    vec3 normal = normalize(mat3(transpose(inverse(view))) * texture(gNormal, TexCoords).xyz);
    vec3 randomVec = normalize(texture(noiseTexture, TexCoords * noiseScale).xyz);

    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;
    const float radius = 0.5;
    const float bias = 0.025;

    for(int i = 0; i < 64; ++i)
    {
        vec3 samplePos = fragPos + (TBN * samples[i]) * radius;

        vec4 offset = vec4(samplePos, 1.0);
        offset = projection * offset;
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;

        if (offset.x < 0.0 || offset.x > 1.0 || offset.y < 0.0 || offset.y > 1.0) {
            continue;
        }

        vec3 sampleWorldPos = texture(gPosition, offset.xy).xyz;
        vec3 sampleViewPos = (view * vec4(sampleWorldPos, 1.0)).xyz;
        float sampleDepth = sampleViewPos.z;

        float depthDelta = abs(fragPos.z - sampleDepth);
        float rangeCheck = smoothstep(0.0, 1.0, radius / max(depthDelta, 0.0001));
        occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
    }

    fragColor = 1.0 - (occlusion / 64.0);
}
