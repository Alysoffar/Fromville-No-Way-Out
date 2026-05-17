#version 330 core
out vec3 viewRay;
uniform mat4 invViewRot;
uniform mat4 invProj;

void main() {
    // Generate fullscreen triangle
    vec2 pos;
    if(gl_VertexID == 0) pos = vec2(-1.0, -1.0);
    else if(gl_VertexID == 1) pos = vec2(3.0, -1.0);
    else pos = vec2(-1.0, 3.0);
    gl_Position = vec4(pos, 1.0, 1.0);
    
    vec4 rayClip = vec4(pos, -1.0, 1.0);
    vec4 rayEye = invProj * rayClip;
    rayEye = vec4(rayEye.xy, -1.0, 0.0);
    viewRay = (invViewRot * rayEye).xyz;
}
