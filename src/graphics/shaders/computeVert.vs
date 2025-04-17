#version 460 core
layout(location = 0) in vec3 aPos;

struct Particle {
    vec4 pos;
    vec4 base;
    vec4 color;
};

layout(std430, binding = 0) buffer Particles {
    Particle particles[];
};

out vec4 vColor;

uniform mat4 uViewProj;

void main() {
    vec3 particlePos = particles[gl_InstanceID].pos.xyz;
    vColor = particles[gl_InstanceID].color;
    gl_Position = uViewProj * vec4(aPos + particlePos, 1.0);
}
