#version 430 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in ivec4 aBoneIDs;
layout(location = 4) in vec4 aWeights;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;
uniform mat4 finalBonesMatrices[MAX_BONES];

void main()
{
    vec4 skinnedPos = vec4(0.0);
    vec3 skinnedNormal = vec3(0.0);
    
    for(int i = 0; i < MAX_BONE_INFLUENCE; i++)
    {
        if(aBoneIDs[i] == -1) 
            continue;
        
        skinnedPos += finalBonesMatrices[aBoneIDs[i]] * vec4(aPos, 1.0) * aWeights[i];
        
        // Approximate normal skinning. Technically requires inverse transpose of bone matrix,
        // but for rigid transformations bone matrix is fine.
        skinnedNormal += mat3(finalBonesMatrices[aBoneIDs[i]]) * aNormal * aWeights[i];
    }
    
    // Fallback if no bones affect this vertex
    if (skinnedPos == vec4(0.0)) {
        skinnedPos = vec4(aPos, 1.0);
        skinnedNormal = aNormal;
    }

    FragPos = vec3(model * skinnedPos);
    Normal = mat3(transpose(inverse(model))) * skinnedNormal;
    TexCoords = aTexCoords;
    
    gl_Position = projection * view * model * skinnedPos;
}
