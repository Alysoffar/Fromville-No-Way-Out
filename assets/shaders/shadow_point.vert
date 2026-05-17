#version 450 core

layout(location=0) in vec3 aPos;
layout(location=5) in ivec4 aBoneIDs;
layout(location=6) in vec4 aBoneWeights;

uniform mat4 model;

layout(std140, binding = 0) uniform BoneMatrices {
    mat4 bones[100];
};

void main() {
    bool hasBones = false;
    mat4 boneTransform = mat4(0.0);
    for(int i = 0; i < 4; i++) {
        if(aBoneIDs[i] != -1) {
            hasBones = true;
            boneTransform += bones[aBoneIDs[i]] * aBoneWeights[i];
        }
    }
    
    vec4 localPosition = vec4(aPos, 1.0);
    if(hasBones) {
        localPosition = boneTransform * localPosition;
    }
    
    gl_Position = model * localPosition; // world space
}
