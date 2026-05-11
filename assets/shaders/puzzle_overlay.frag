#version 330 core

out vec4 FragColor;

uniform vec3 uTint;
uniform float uAlpha;
uniform float uInvestigationMode;
uniform float uTerrorLevel;
uniform float uModalAge;

// Soft, foggy overlay: gentle vignette + subtle desaturation when tension rises.
void main() {
    vec2 uv = gl_FragCoord.xy / vec2(1280.0, 720.0);
    vec2 center = vec2(0.5, 0.45);
    float dist = distance(uv, center);
    float vignette = smoothstep(0.65, 0.95, dist);

    float investigation = clamp(uInvestigationMode, 0.0, 1.0);
    float terror = clamp(uTerrorLevel, 0.0, 1.0);

    vec3 fogColor = mix(uTint, vec3(0.62, 0.68, 0.72), investigation * 0.8);
    fogColor = mix(fogColor, fogColor * 0.92, terror * 0.35);

    float shimmer = 0.02 * sin(uModalAge * 3.0 + uv.x * 6.0) * investigation;
    fogColor += vec3(shimmer);

    float alpha = uAlpha * (0.60 + investigation * 0.25) * (1.0 - vignette * 0.6);

    FragColor = vec4(fogColor, alpha);
}