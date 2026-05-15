#version 450 core
out vec4 FragColor;
in vec2 TexCoords;
uniform sampler2D overlay;
uniform vec4 tintColor;
void main() {
    vec4 texColor = texture(overlay, TexCoords);
    FragColor = texColor * tintColor;
}
