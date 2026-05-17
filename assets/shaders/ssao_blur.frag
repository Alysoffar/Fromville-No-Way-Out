#version 450 core
out float fragColor;

in vec2 TexCoords;

layout(binding = 0) uniform sampler2D ssaoInput;

void main() 
{
    vec2 texelSize = 1.0 / vec2(textureSize(ssaoInput, 0));
    float result = 0.0;
    for (int x = 0; x < 4; ++x) 
    {
        for (int y = 0; y < 4; ++y) 
        {
            vec2 centered = vec2(float(x) - 1.5, float(y) - 1.5);
            vec2 offset = centered * texelSize;
            result += texture(ssaoInput, TexCoords + offset).r;
        }
    }
    fragColor = result * (1.0 / 16.0);
}
