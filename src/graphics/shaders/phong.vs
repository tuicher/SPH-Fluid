#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 uModelView;
uniform mat4 uProjection;
uniform mat3 uNormalMat;

out vec3 vNormal;       // la normal en el FS
out vec3 vWorldPos;     // posición en espacio mundo (opcional)

void main()
{
    vec4 vertPos4 = uModelView * vec4(aPos, 1.0);
    vWorldPos = vec3(vertPos4) / vertPos4.w;
    vNormal = normalize(uNormalMat * aNormal);
    gl_Position = uProjection * vertPos4;
}