#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 lightDir;
uniform vec3 viewPos;
uniform bool hasTexture;
uniform sampler2D grassTexture;
uniform vec2 resolution;
uniform float objectType;

void main()
{
    vec3 baseColor;
    
    if (objectType == 3.0) {
        // Mountains
        vec3 rockLow = vec3(0.40, 0.33, 0.25);
        vec3 rockHigh = vec3(0.78, 0.76, 0.74);
        float mountainH = clamp(FragPos.y / 90.0, 0.0, 1.0);
        baseColor = mix(rockLow, rockHigh, mountainH);
        
        if (FragPos.y > 65.0) {
            float snowBlend = clamp((FragPos.y - 65.0) / 10.0, 0.0, 1.0);
            baseColor = mix(baseColor, vec3(0.95, 0.95, 0.97), snowBlend);
        }
    } else {
        // Terrain (objectType == 0.0)
        if (hasTexture) {
            baseColor = texture(grassTexture, TexCoords).rgb;
        } else {
            baseColor = vec3(0.3, 0.8, 0.2); 
            float pattern = sin(FragPos.x * 0.8) * sin(FragPos.z * 0.8);
            baseColor *= (1.0 + pattern * 0.1); 
        }

        // Height-based color tinting
        vec3 colorLow = vec3(0.25, 0.55, 0.15);
        vec3 colorHigh = vec3(0.45, 0.72, 0.20);
        float hFactor = clamp(FragPos.y / 12.0, 0.0, 1.0);
        baseColor *= mix(colorLow, colorHigh, hFactor);
    }

    vec3 albedo = baseColor;
    vec3 norm = normalize(Normal);

    // Slope-based tinting (terrain only)
    if (objectType == 0.0) {
        vec3 slopeColorLow = vec3(0.40, 0.33, 0.25);
        vec3 slopeColorHigh = vec3(0.78, 0.76, 0.74);
        float slopeH = clamp(FragPos.y / 90.0, 0.0, 1.0);
        vec3 slopeColor = mix(slopeColorLow, slopeColorHigh, slopeH);

        float slope = 1.0 - max(dot(norm, vec3(0.0, 1.0, 0.0)), 0.0);
        float slopeFactor = smoothstep(0.1, 0.4, slope); 
        albedo = mix(albedo, slopeColor, slopeFactor);
    }

    // Ambient Occlusion
    float ao = clamp(norm.y * 0.5 + 0.5, 0.0, 1.0);

    // Blinn-Phong Lighting
    vec3 lDir = normalize(lightDir);
    vec3 ambient = 0.3 * albedo * ao;
    
    float diff = max(dot(norm, lDir), 0.0);
    vec3 diffuse = diff * albedo;
    
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lDir + viewDir);  
    float spec = pow(max(dot(norm, halfwayDir), 0.0), 16.0);
    vec3 specular = vec3(0.05) * spec; 
    
    vec3 result = ambient + diffuse + specular;

    // Fog
    float distance = length(viewPos - FragPos);
    float fogStart = 80.0;
    float fogEnd = 200.0;
    float fogFactor = clamp((distance - fogStart) / (fogEnd - fogStart), 0.0, 1.0);
    vec3 fogColor = vec3(0.529, 0.808, 0.922); 
    
    result = mix(result, fogColor, fogFactor);

    // Vignette
    vec2 uv = gl_FragCoord.xy / resolution;
    result *= (1.0 - 0.3 * pow(length(uv - 0.5) * 1.6, 2.0));

    FragColor = vec4(result, 1.0);
}
