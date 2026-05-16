#version 330 core
out vec4 FragColor;

in vec3 vFragPos;
in vec2 vTexCoord;

uniform sampler2D uGrassTexture;

uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uAmbient;
uniform float uDiffuseStrength;
uniform vec3 uViewPos;
uniform vec3 uFogColor;
uniform float uFogDensity;

void main() {
    vec4 texColor = texture(uGrassTexture, vTexCoord);
    
    // Alpha discard
    if (texColor.a < 0.1) discard;
    // Black discard fallback
    if (texColor.r < 0.05 && texColor.g < 0.05 && texColor.b < 0.05) discard;

    vec3 normal = vec3(0.0, 1.0, 0.0); // Upward normal for grass
    vec3 lightDir = normalize(-uLightDir);
    
    // Simple diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * uLightColor * uDiffuseStrength * texColor.rgb;
    
    vec3 litColor = (uAmbient * texColor.rgb) + diffuse;

    float dist = length(uViewPos - vFragPos);
    float fogFactor = exp(-uFogDensity * dist * dist);
    vec3 finalColor = mix(uFogColor, litColor, clamp(fogFactor, 0.0, 1.0));

    FragColor = vec4(finalColor, 1.0);
}
