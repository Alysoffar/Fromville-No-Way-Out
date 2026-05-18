#version 330 core
layout (location = 0) in vec2 aPos;
uniform vec2 uPosition;  // center in NDC
uniform vec2 uSize;      // half-width, half-height in NDC
void main() {
    vec2 pos = uPosition + aPos * uSize;
    gl_Position = vec4(pos, 0.0, 1.0);
}
