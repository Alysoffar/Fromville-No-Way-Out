#version 330 core
out vec4 FragColor;

in vec3 vWorldDir;

uniform vec3 uSunDir;
uniform float uDayFactor;
uniform float uDayTime;

void main() {
    vec3 dir = normalize(vWorldDir);
    
    // Night: Deep navy at horizon, near-black at zenith
    vec3 nightZenith = vec3(0.01, 0.01, 0.02);
    vec3 nightHorizon = vec3(0.04, 0.05, 0.1);
    
    // Day: Pale gray-blue
    vec3 dayZenith = vec3(0.4, 0.5, 0.7);
    vec3 dayHorizon = vec3(0.7, 0.8, 0.9);
    
    float factor = clamp(dir.y, 0.0, 1.0);
    vec3 nightColor = mix(nightHorizon, nightZenith, factor);
    vec3 dayColor = mix(dayHorizon, dayZenith, factor);
    
    vec3 skyColor = mix(nightColor, dayColor, uDayFactor);
    
    // Simple sun disc
    float sunDot = dot(dir, normalize(uSunDir));
    float sunDisk = smoothstep(0.998 - uDayTime * 0.001, 0.999 + uDayTime * 0.001, sunDot);
    skyColor += vec3(1.0, 0.9, 0.7) * sunDisk * uDayFactor;
    
    FragColor = vec4(skyColor, 1.0);
}
