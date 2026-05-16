#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aInstancePos;

uniform mat4 uView;
uniform mat4 uProjection;
uniform float uTime;
uniform mat3 uBillboard;

out vec3 vFragPos;
out vec2 vTexCoord;

void main() {
    // Wind effect: only apply to top of the blade (root anchoring)
    float wind = sin(uTime * 2.0 + aInstancePos.x * 0.5) * 0.3 + sin(uTime * 1.3 + aInstancePos.z * 0.7) * 0.1;
    vec3 localPos = aPosition;
    localPos.x += wind * aPosition.y; 

    vec3 worldPos = (uBillboard * localPos) + aInstancePos;
    vFragPos = worldPos;
    vTexCoord = aTexCoord;
    gl_Position = uProjection * uView * vec4(worldPos, 1.0);
}
