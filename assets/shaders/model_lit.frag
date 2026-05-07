#version 330 core
out vec4 FragColor;

in vec3 vertexColor;
in vec3 normal;
in vec3 FragPos;

uniform vec3 lightDir;
uniform vec3 viewPos;

void main()
{
    vec3 baseColor = vertexColor;
    
    vec3 norm = normalize(normal);
    vec3 lDir = normalize(lightDir);

    float ao = clamp(norm.y * 0.5 + 0.5, 0.0, 1.0);
    vec3 ambient = 0.35 * baseColor * ao;

    float diff = max(dot(norm, lDir), 0.0);
    vec3 diffuse = diff * baseColor;

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

    FragColor = vec4(result, 1.0);
}
