#version 330 core
layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 col;
uniform mat4 viewProj;
out vec3 vCol;
void main() {
    gl_Position = viewProj * vec4(pos, 1.0);
    vCol = col;
}
