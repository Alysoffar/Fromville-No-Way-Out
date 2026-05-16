#version 330 core

layout(location = 0) in vec3 aPos;

uniform mat4 uProjection;
uniform mat4 uView;
uniform mat4 uModel;

out vec3 vWorldPos;

void main() {
    vWorldPos = (uModel * vec4(aPos, 1.0)).xyz;
    vec4 pos = uProjection * uView * uModel * vec4(aPos, 1.0);
    gl_Position = pos.xyww; // force depth to 1.0 (always behind everything)
}
