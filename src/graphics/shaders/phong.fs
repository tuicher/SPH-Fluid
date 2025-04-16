#version 330 core
in vec3 vNormal;
in vec3 vWorldPos;

out vec4 FragColor;

// Uniformes para Phong
uniform vec3 uViewPos;    // posición de la cámara
uniform vec3 uLightPos;   // posición de la luz
uniform vec3 uLightColor; 
uniform vec3 uObjectColor;

void main()
{
    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightPos - vWorldPos);

    float ambientStrength = 0.5;
    vec3 ambient = ambientStrength * uLightColor;

    float lambertian = max(dot(N, L), 0.0);
    float specular = 0.0;
    
    if(lambertian > 0.0)
    {
        vec3 R = reflect(-L, N);
        vec3 V = normalize(-vWorldPos);
        float specAngle = max(dot(R, V), 0.0);
        specular = pow(specAngle, 32);
    }

    gl_FragColor = vec4( (ambient + 0.5 * lambertian + specular) * uObjectColor, 1.0);
}
