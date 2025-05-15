#version 460 core
in vec3  vNormal;
in vec3  vViewDir;
in vec4  vColor;

out vec4 FragColor;

uniform vec3 uLightDir = normalize(vec3(-0.5, -1.0, -0.3));

void main() {
    //float NdotL  = clamp( dot(vNormal, -uLightDir), 0.0, 1.0 );
    //float fresnel = pow( 1.0 - dot(vNormal, vViewDir), 3.0 );

    //vec3 baseCol = vColor.rgb * 0.6 + vec3(0.2,0.4,0.8)*0.4;
    //vec3 diffuse = baseCol * NdotL;
    //vec3 spec    = vec3(0.3) * fresnel;

    //vec3 rgb = diffuse + spec;
    FragColor = vec4(vColor.rgb, 0.6);
}
