#version 330 core
layout(location = 0) in vec3 aPosition;

uniform mat4 uView;
uniform mat4 uProjection;
uniform mat4 uModel;

out vec3 vWorldDir;

void main() {
    vWorldDir = aPosition;
    gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);
}
