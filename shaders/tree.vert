#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aColor;
layout (location = 3) in vec2 aUV;
// Instance model matrix (takes locations 4, 5, 6, 7)
layout (location = 4) in mat4 aInstanceModel;

out vec3 FragPos;
out vec3 Normal;
out vec3 Color;
out vec2 TexCoords;

uniform mat4 uView;
uniform mat4 uProjection;

void main()
{
    mat3 normalMatrix = transpose(inverse(mat3(aInstanceModel)));
    Normal = normalize(normalMatrix * aNormal);
    
    Color = aColor;
    TexCoords = aUV;
    
    vec4 worldPos = aInstanceModel * vec4(aPos, 1.0);
    FragPos = vec3(worldPos);

    gl_Position = uProjection * uView * worldPos;
}
