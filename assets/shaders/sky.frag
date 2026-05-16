#version 330 core
out vec4 FragColor;
in vec3 viewRay;

uniform vec3 lightDir;
uniform vec2 resolution;

void main() {
    vec3 dir = normalize(viewRay);
    
    // Gradient
    vec3 skyBottom = vec3(0.74, 0.88, 0.97);
    vec3 skyTop = vec3(0.18, 0.38, 0.72);
    float yFactor = clamp(dir.y, 0.0, 1.0);
    vec3 skyColor = mix(skyBottom, skyTop, yFactor);
    
    // Sun
    vec3 lDir = normalize(lightDir);
    float sunDot = dot(dir, lDir);
    
    vec3 sunColor = vec3(1.0, 0.97, 0.85);
    float sunFactor = smoothstep(0.997, 0.999, sunDot);
    
    vec3 haloColor = vec3(1.0, 0.75, 0.3);
    float haloFactor = smoothstep(0.980, 0.997, sunDot);
    
    vec3 finalColor = skyColor + haloColor * haloFactor + sunColor * sunFactor;
    
    // Procedural Clouds
    vec2 uv = gl_FragCoord.xy / resolution.xy;
    
    float d1 = length((uv - vec2(0.3, 0.7)) * vec2(1.0, 2.5));
    float cloud1 = smoothstep(0.15, 0.05, d1);
    
    float d2 = length((uv - vec2(0.7, 0.6)) * vec2(1.0, 3.0));
    float cloud2 = smoothstep(0.12, 0.04, d2);
    
    float d3 = length((uv - vec2(0.5, 0.8)) * vec2(1.0, 2.0));
    float cloud3 = smoothstep(0.18, 0.08, d3);
    
    float totalCloud = clamp(cloud1 + cloud2 + cloud3, 0.0, 1.0);
    
    float cloudGrad = smoothstep(0.5, 0.9, uv.y);
    vec3 cloudColor = mix(vec3(0.88), vec3(1.0), cloudGrad);
    
    finalColor = mix(finalColor, cloudColor, totalCloud * 0.9);

    // Vignette
    finalColor *= (1.0 - 0.4 * pow(length(uv - 0.5) * 1.6, 2.0));

    FragColor = vec4(finalColor, 1.0);
}
