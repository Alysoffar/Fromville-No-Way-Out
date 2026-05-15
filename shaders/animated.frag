#version 430 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 lightDir;
uniform vec3 lightColor;
uniform float fogDensity;
uniform vec3 fogColor;
uniform vec3 viewPos;
uniform sampler2D diffuseMap;
uniform bool u_UseTexture;

void main()
{
    vec4 texColor;
    if (u_UseTexture) {
        texColor = texture(diffuseMap, TexCoords);
        if(texColor.a < 0.1)
            discard;
    } else {
        texColor = vec4(0.8, 0.8, 0.8, 1.0); // Light gray fallback
    }
        
    vec3 norm = normalize(Normal);
    vec3 lightDirNorm = normalize(lightDir);
    
    // Ambient
    vec3 ambient = 0.3 * lightColor;
    
    // Diffuse
    float diff = max(dot(norm, lightDirNorm), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Specular (Blinn-Phong)
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lightDirNorm + viewDir);  
    float spec = pow(max(dot(norm, halfwayDir), 0.0), 32.0);
    vec3 specular = 0.5 * spec * lightColor;
    
    vec3 finalLighting = (ambient + diffuse + specular) * texColor.rgb;
    
    // Fog calculation (exponential distance fog)
    float distance = length(viewPos - FragPos);
    float fogFactor = exp(-fogDensity * distance);
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    
    // Mix fog color with lit color
    vec3 finalColor = mix(fogColor, finalLighting, fogFactor);
    
    FragColor = vec4(finalColor, 1.0);
}
