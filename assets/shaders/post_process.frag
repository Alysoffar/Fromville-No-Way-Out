#version 450 core

in vec2 TexCoords;
out vec4 fragColor;

uniform sampler2D hdrBuffer;
uniform sampler2D bloomBlur;
uniform sampler2D ssaoBuffer;

uniform float exposure;
uniform float desaturation;
uniform float vignetteStrength;
uniform float screenShakeIntensity;
uniform float redChromaticAberration;
uniform float symbolSightStrength;
uniform float uvRippleStrength;
uniform vec3 colorGrade;
uniform vec3 fogColorGrade;
uniform float time;
uniform vec2 resolution;

void main() {
	vec2 uv = TexCoords;

	if (screenShakeIntensity > 0.0) {
		uv += vec2(sin(time * 47.3) * 0.003, cos(time * 31.7) * 0.003) * screenShakeIntensity;
	}

	if (uvRippleStrength > 0.0) {
		float edge = length(uv - 0.5) * 2.0;
		uv += vec2(sin(uv.y * 20.0 + time * 5.0) * 0.002, 0.0) * uvRippleStrength * edge;
	}

	vec3 color;
	if (redChromaticAberration > 0.0) {
		float offset = redChromaticAberration * 0.005;
		float rSample = texture(hdrBuffer, uv + vec2(offset, 0.0)).r;
		float gSample = texture(hdrBuffer, uv).g;
		float bSample = texture(hdrBuffer, uv - vec2(offset, 0.0)).b;
		color = vec3(rSample, gSample, bSample);
	} else {
		color = texture(hdrBuffer, uv).rgb;
	}

	color *= exposure;
	vec3 x = color;
	color = (x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14);
	color = clamp(color, 0.0, 1.0);

	color += texture(bloomBlur, uv).rgb * 0.15;

	float luma = dot(color, vec3(0.299, 0.587, 0.114));
	color = mix(color, vec3(luma), desaturation);

	if (symbolSightStrength > 0.0) {
		float L = dot(color, vec3(0.299, 0.587, 0.114));
		vec3 sepia = vec3(L * 1.2, L * 1.0, L * 0.7);
		color = mix(color, sepia, symbolSightStrength * 0.7);
	}

	color *= colorGrade;

	float dist = length(TexCoords - 0.5) * 2.0;
	float vignette = 1.0 - smoothstep(0.5, 1.4, dist) * vignetteStrength;
	color *= vignette;

	color = pow(color, vec3(1.0 / 2.2));
	fragColor = vec4(color, 1.0);
}

