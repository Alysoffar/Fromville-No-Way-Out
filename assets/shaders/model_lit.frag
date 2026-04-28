#version 330 core
out vec4 FragColor;

in vec3 vertexColor;
in vec3 normal;

// Optional global tint for the model; defined to avoid uniform-not-found warnings
uniform vec3 color = vec3(1.0);

void main()
{
    // Fake sunlight coming from the top-right
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0)); 
    vec3 norm = normalize(normal);

    // Calculate how directly the light hits the surface (0.0 to 1.0)
    float diff = max(dot(norm, lightDir), 0.0);

    // Add a little ambient light so the shadows aren't pitch black
    float ambient = 0.3;

    // Multiply the vertex color by our light calculations and global tint
    vec3 finalColor = vertexColor * (diff + ambient) * color;

    FragColor = vec4(finalColor, 1.0);
}
