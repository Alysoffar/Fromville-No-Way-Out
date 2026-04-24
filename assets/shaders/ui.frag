#version 450 core
out vec4 FragColor;
in vec2 TexCoords;
uniform sampler2D overlay;
void main() {
    vec4 texColor = texture(overlay, TexCoords);
    if (texColor.a < 0.01) {
        FragColor = vec4(1.0, 0.0, 0.0, 0.5); // Fallback semi-transparent red
    } else {
        FragColor = texColor;
    }
}
