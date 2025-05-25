#version 460 core
layout(location = 0) in vec3 aPosition;  // posición del vértice en la esfera modelo (origen)
layout(location = 1) in vec3 aNormal;    // normal del vértice en la esfera modelo

// Bloque de almacenamiento (SSBO) con las posiciones de partículas (binding 0)
struct Particle {
    vec4 pos;
    vec4 base;
};

layout(std430, binding = 0) buffer Particles {
    Particle particles[];
};

uniform mat4 uViewProj;  // matriz combinada View-Projection (para transformar a clip space)
out vec3 vNormal;        // normal a interpolar hacia el fragment shader

void main() {
    // Obtener la posición de esta instancia (según gl_InstanceID)
    vec3 instancePos = particles[gl_InstanceID].pos.xyz;
    // Trasladar el vértice de la esfera unitaria a la posición de la partícula
    vec3 worldPos = instancePos + aPosition;   // suponiendo escala = 1
    // Transformar a espacio de clip
    gl_Position = uViewProj * vec4(worldPos, 1.0);
    // La normal permanece igual (solo traslación, sin rotación ni escala no uniforme)
    vNormal = mat3(uViewProj) * aNormal;  // transformar normal a espacio cámara (aprox)
}
