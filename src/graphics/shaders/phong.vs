#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 uMVP;
uniform mat4 uModel;    // para transformar normales en espacio mundo/objeto
uniform mat3 uNormalMat; // matriz de normales (inversa transpuesta de model)

out vec3 vNormal;       // la normal en el FS
out vec3 vWorldPos;     // posición en espacio mundo (opcional)

void main()
{
    // Transformar la posición
    gl_Position = uMVP * vec4(aPos, 1.0);

    // Suponiendo que deseas la normal en espacio mundo 
    // (o en espacio de vista, según tu pipeline)
    vNormal   = normalize(uNormalMat * aNormal);

    // También puedes obtener la posición en espacio mundo
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vWorldPos = worldPos.xyz;
}
