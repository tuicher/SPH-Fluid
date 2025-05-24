#version 460 core
layout(location = 0) in vec3 aPos;

struct Particle {
    vec4 x;      // posición actual
    vec4 v;      // velocidad  (sin uso en este shader)
    vec4 p;      // posición predicha (sin uso aquí)
    vec4 color;  // RGBA
    vec4 meta;   // datos extra
};

layout(std430, binding = 0) readonly buffer Particles {
    Particle particles[];
};

out vec3  vNormal;
out vec3  vViewDir;
out vec4  vColor;

uniform mat4 uViewProj;
uniform mat4 uView;

void main()
{
    uint id      = gl_InstanceID;
    vec3 center  = particles[id].x.xyz;   // <- CAMBIO: usar x, no pos
    vec3 worldPos = aPos + center;

    gl_Position = uViewProj * vec4(worldPos, 1.0);

    vNormal  = normalize(aPos);
    vViewDir = normalize( (inverse(uView) * vec4(0,0,0,1)).xyz - worldPos );
    //vColor   = particles[id].color;
    float val = particles[id].meta.z;
    vColor   = vec4(val,val,val,1);
}