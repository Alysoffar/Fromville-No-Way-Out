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
    // The G-Buffer contains world space positions, convert them to view space
    vec3 worldPos = texture(gPosition, TexCoords).xyz;
    vec3 fragPos = (view * vec4(worldPos, 1.0)).xyz;
    
    // Normal is in world space, convert to view space
    vec3 worldNormal = texture(gNormal, TexCoords).xyz;
    vec3 normal = normalize(mat3(transpose(inverse(view))) * worldNormal);
    
    vec3 randomVec = normalize(texture(noiseTexture, TexCoords * noiseScale).xyz);
    
    // Gram-Schmidt process
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);
    
    float occlusion = 0.0;
    float radius = 0.5;
    float bias = 0.025;
    
    for(int i = 0; i < 64; ++i)
    {
        // Get sample position
        vec3 samplePos = TBN * samples[i];
        samplePos = fragPos + samplePos * radius; 
        
        // Project sample position to screen space
        vec4 offset = vec4(samplePos, 1.0);
        offset = projection * offset;
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5; // Transform to [0,1]
        
        // Get sample depth
        vec3 sampleWorldPos = texture(gPosition, offset.xy).xyz;
        vec3 sampleViewPos = (view * vec4(sampleWorldPos, 1.0)).xyz;
        float sampleDepth = sampleViewPos.z;
        
        // Range check & accumulate
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
    }
    
    occlusion = 1.0 - (occlusion / 64.0);
    fragColor = occlusion;
}
