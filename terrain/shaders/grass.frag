#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec2 TexCoords;
in vec3 Normal;

uniform sampler2D uGrassTexture;

uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uAmbient;
uniform float uDiffuseStrength;
uniform vec3 uViewPos;

uniform vec3 uFogColor;
uniform float uFogDensity;

void main()
{
    vec4 texColor = texture(uGrassTexture, TexCoords);

    // Discard truly transparent pixels
    if (texColor.a < 0.15) discard;

    // Discard dark/black background pixels (handles PNGs with black instead of alpha)
    float brightness = dot(texColor.rgb, vec3(0.299, 0.587, 0.114));
    if (brightness < 0.08) discard;

    vec3 color = texColor.rgb;

    // Lighting
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(-uLightDir);

    // Ambient — driven by day/night cycle uniform
    vec3 ambient = uAmbient * color;

    // Diffuse - use abs(dot) for two-sided lighting on billboards
    float diff = max(abs(dot(norm, lightDir)), 0.0);
    vec3 diffuse = diff * uLightColor * uDiffuseStrength * color;

    // Specular (minimal for grass)
    vec3 viewDir = normalize(uViewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), 16.0);
    vec3 specular = vec3(0.02) * spec * uLightColor;

    vec3 litColor = ambient + diffuse + specular;

    // Distance fog
    float fragDistance = length(FragPos - uViewPos);
    float fogFactor = exp(-uFogDensity * fragDistance * fragDistance * 0.5);
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    vec3 finalColor = mix(uFogColor, litColor, fogFactor);

    FragColor = vec4(finalColor, 1.0);
}
