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
    // 1) Ambient
    float ambientStrength = 0.2;
    vec3 ambient = ambientStrength * uLightColor;

    // 2) Diffuse
    vec3 norm = normalize(vNormal);
    vec3 lightDir = normalize(uLightPos - vWorldPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * uLightColor;

    // 3) Specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(uViewPos - vWorldPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * uLightColor;

    //vec3 result = (ambient + diffuse + specular) * uObjectColor;
    vec3 result = (ambient + diffuse) * uObjectColor;

    //vec3 result = vNormal;
    FragColor = vec4(result, 1.0);
}
