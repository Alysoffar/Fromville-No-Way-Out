#version 330 core
out vec4 FragColor;

in vec3 vertexColor;
in vec3 normal;
in vec3 FragPos;

uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uAmbient;
uniform float uDiffuseStrength;
uniform vec3 uViewPos;
uniform vec3 uFogColor;
uniform float uFogDensity;

uniform vec3 color = vec3(1.0);
uniform bool uUseFlatColor = false;

void main()
{
    vec3 baseColor = uUseFlatColor ? color : vertexColor;
    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(uLightDir);

    float ao = clamp(norm.y * 0.5 + 0.5, 0.0, 1.0);
    vec3 ambient = uAmbient * baseColor * ao;

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * uLightColor * uDiffuseStrength * baseColor;

    vec3 viewDir = normalize(uViewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), 16.0);
    vec3 specular = vec3(0.05) * spec * uLightColor;

    vec3 result = ambient + diffuse + specular;

    // Exponential Fog
    float distance = length(uViewPos - FragPos);
    float fogFactor = exp(-pow(distance * uFogDensity, 2.0));
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    
    result = mix(uFogColor, result, fogFactor);

    FragColor = vec4(result, 1.0);
}
