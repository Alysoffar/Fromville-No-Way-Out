#version 450 core

layout(location = 0) in vec2 aQuad;
layout(location = 1) in vec3 iPosition;
layout(location = 2) in vec4 iColor;
layout(location = 3) in float iSize;
layout(location = 4) in float iRotation;

uniform mat4 view;
uniform mat4 projection;

out vec4 vColor;
out vec2 vUV;

void main() {
	vec3 right = vec3(view[0][0], view[1][0], view[2][0]);
	vec3 up = vec3(view[0][1], view[1][1], view[2][1]);

	float c = cos(iRotation);
	float s = sin(iRotation);
	vec2 rotatedQuad = vec2(c * aQuad.x - s * aQuad.y, s * aQuad.x + c * aQuad.y);

	vec3 worldPos = iPosition + right * rotatedQuad.x * iSize + up * rotatedQuad.y * iSize;
	gl_Position = projection * view * vec4(worldPos, 1.0);

	vColor = iColor;
	vUV = aQuad * 0.5 + 0.5;
}

