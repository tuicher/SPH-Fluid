#version 460 core
in vec3 vNormal;
out vec4 FragColor;

// Uniformes para iluminación sencilla
uniform vec3 uLightDir = vec3(0.0, 1.0, 1.0);  // dirección de la luz (ej. desde arriba-frente)
uniform vec3 uColor    = vec3(0.2, 0.6, 1.0);  // color base de las esferas
uniform vec3 uLightColor = vec3(1.0);          // color de la luz (blanco)
uniform float uAmbient  = 0.2;

void main() {
    // Intensidad difusa según el ángulo con la luz (Phong básico)
    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightDir);
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * uLightColor * uColor;
    vec3 ambient = uAmbient * uColor;
    FragColor = vec4(diffuse + ambient, 1.0);
}
