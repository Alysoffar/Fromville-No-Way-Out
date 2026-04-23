#version 450 core

in vec2 TexCoords;
out vec4 fragColor;

uniform sampler2D image;
uniform bool horizontal;

void main() {
    vec2 texel = 1.0 / vec2(textureSize(image, 0));
    float w0 = 0.227027;
    float w1 = 0.316216;
    float w2 = 0.070270;

    vec3 result = texture(image, TexCoords).rgb * w0;
    if (horizontal) {
        result += texture(image, TexCoords + vec2(texel.x, 0.0)).rgb * w1;
        result += texture(image, TexCoords - vec2(texel.x, 0.0)).rgb * w1;
        result += texture(image, TexCoords + vec2(2.0 * texel.x, 0.0)).rgb * w2;
        result += texture(image, TexCoords - vec2(2.0 * texel.x, 0.0)).rgb * w2;
    } else {
        result += texture(image, TexCoords + vec2(0.0, texel.y)).rgb * w1;
        result += texture(image, TexCoords - vec2(0.0, texel.y)).rgb * w1;
        result += texture(image, TexCoords + vec2(0.0, 2.0 * texel.y)).rgb * w2;
        result += texture(image, TexCoords - vec2(0.0, 2.0 * texel.y)).rgb * w2;
    }

    fragColor = vec4(result, 1.0);
}
