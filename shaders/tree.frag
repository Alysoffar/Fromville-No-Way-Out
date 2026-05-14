#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec3 Color;
in vec2 TexCoords;

uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uAmbient;
uniform float uDiffuseStrength;
uniform vec3 uViewPos;

uniform vec3 uFogColor;
uniform float uFogDensity;

void main()
{
    vec3 baseColor = Color;

    vec3 dx = dFdx(FragPos);
    vec3 dy = dFdy(FragPos);
    vec3 norm = normalize(cross(dx, dy));
    
    vec3 lightDir = normalize(uLightDir);

    vec3 ambient = uAmbient * baseColor;

    // Wrap lighting for softer tree shading (prevents pure black shadows)
    float diff = max(dot(norm, lightDir) * 0.6 + 0.4, 0.0);
    vec3 diffuse = diff * uLightColor * uDiffuseStrength * baseColor;

    vec3 viewDir = normalize(uViewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), 16.0);
    vec3 specular = vec3(0.05) * spec * uLightColor;

    vec3 litColor = ambient + diffuse + specular;

    float distance = length(uViewPos - FragPos);
    float fogFactor = exp(-pow(distance * uFogDensity, 2.0));
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    vec3 finalColor = mix(uFogColor, litColor, fogFactor);

    FragColor = vec4(finalColor, 1.0);
}
