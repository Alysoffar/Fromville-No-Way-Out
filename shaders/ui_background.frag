#version 330 core
in vec2 TexCoord;
out vec4 FragColor;
uniform sampler2D uBackground;
uniform float uDarkness; // 0.0 = full bright, 1.0 = pure black overlay
void main() {
    vec4 tex = texture(uBackground, TexCoord);
    FragColor = mix(tex, vec4(0.0, 0.0, 0.0, 1.0), uDarkness);
}
