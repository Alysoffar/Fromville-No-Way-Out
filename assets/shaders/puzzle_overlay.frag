#version 330 core

out vec4 FragColor;

uniform vec3 uTint;
uniform float uAlpha;
uniform float uInvestigationMode;
uniform float uTerrorLevel;
uniform float uModalAge;
uniform float u_SymbolVisibility;
uniform float u_SketchOverlayAlpha;
uniform float u_InvisibilityDistortion;

// Soft, foggy overlay: gentle vignette + subtle desaturation when tension rises.
void main() {
    vec2 uv = gl_FragCoord.xy / vec2(1280.0, 720.0);
    float distortion = clamp(u_InvisibilityDistortion, 0.0, 1.0);
    if (distortion > 0.001) {
        vec2 d = uv - vec2(0.5, 0.5);
        float ripple = sin(length(d) * 28.0 - uModalAge * 6.0) * 0.004 * distortion;
        uv += normalize(d + vec2(0.0001)) * ripple;
    }

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

    // Jade symbol overlay (screen-space centered glyph hint)
    if (u_SymbolVisibility > 0.001) {
        vec2 symCenter = vec2(0.5, 0.52);
        float sdist = distance(uv, symCenter);
        float symMask = smoothstep(0.18, 0.08, sdist);
        vec3 symColor = vec3(0.6, 0.85, 1.0) * 0.7;
        fogColor = mix(fogColor, fogColor + symColor * 0.45, symMask * u_SymbolVisibility);
        alpha = max(alpha, u_SymbolVisibility * 0.9);
    }

    float sketchAlpha = clamp(u_SketchOverlayAlpha, 0.0, 1.0);
    if (sketchAlpha > 0.001) {
        float hatchA = step(0.78, fract((uv.x + uv.y) * 48.0));
        float hatchB = step(0.84, fract((uv.x - uv.y) * 42.0));
        float hatch = max(hatchA, hatchB) * sketchAlpha;
        vec3 paperTint = vec3(0.86, 0.82, 0.74);
        fogColor = mix(fogColor, paperTint, sketchAlpha * 0.35);
        fogColor = mix(fogColor, fogColor * 0.72, hatch * 0.55);
        alpha = max(alpha, sketchAlpha * 0.75);
    }

    FragColor = vec4(fogColor, alpha);
}
