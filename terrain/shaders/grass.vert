#version 330 core

layout (location = 0) in vec3 aPos;        // billboard template vertex
layout (location = 1) in vec2 aTexCoord;    // billboard UV
layout (location = 2) in vec3 aInstancePos; // per-instance world position

out vec3 FragPos;
out vec2 TexCoords;
out vec3 Normal;

uniform mat4 uView;
uniform mat4 uProjection;
uniform mat3 uBillboard; // camera-facing rotation matrix (computed per frame on CPU)
uniform float uTime;

void main()
{
    // Apply billboard rotation so quad always faces the camera
    vec3 localPos = uBillboard * aPos;

    // Wind: use aPos.y (original local Y before billboard transform) to anchor roots
    float windOffset = sin(uTime * 2.0 + aInstancePos.x * 0.5) * 0.3 * aPos.y;
    windOffset += sin(uTime * 1.3 + aInstancePos.z * 0.7) * 0.1 * aPos.y;
    localPos.x += windOffset;

    // World position = billboard-rotated local + instance position
    vec3 worldPos = localPos + aInstancePos;

    FragPos   = worldPos;
    TexCoords = aTexCoord;
    Normal    = vec3(0.0, 1.0, 0.0);

    gl_Position = uProjection * uView * vec4(worldPos, 1.0);
}
