#version 450 core

in vec4 vColor;
in vec2 vUV;

uniform sampler2D particleTex;

out vec4 fragColor;

void main() {
	fragColor = vColor * texture(particleTex, vUV);
}

