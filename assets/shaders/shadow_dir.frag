#version 450 core

// Empty fragment shader, depth is automatically written
// We explicitly output the fragment depth just in case
layout(location=0) out float fragDepth;

void main() {
    fragDepth = gl_FragCoord.z;
}
