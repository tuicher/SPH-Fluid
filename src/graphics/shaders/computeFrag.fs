#version 460 core
in vec3  vNormal;
in vec3  vViewDir;
in vec4  vColor;

out vec4 FragColor;

void main()
{
    // Normalizar siempre antes de usar
    vec3 N = normalize(vNormal);
    vec3 L = normalize(vec3(1.0,1.0,1.0));

    // Producto escalar y saturación
    float NdotL = max(dot(N, L), 0.0);

    //vec3 baseCol = vColor.rgb * 0.6 + vec3(0.2, 0.4, 0.8) * 0.4;

    vec3 baseCol = vColor.rgb;
    // Difuso de Lambert
    vec3 diffuse = (baseCol * NdotL * 0.6) + (0.4 * baseCol);

    // Sin spec, sin Fresnel
    FragColor = vec4(diffuse, 1.0);

    //FragColor = vec4(baseCol, 1.0);
}