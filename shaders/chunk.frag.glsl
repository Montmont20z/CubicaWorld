#version 330 core
in vec3 vNormal;
in float vAO;
out vec4 FragColor;

uniform vec3 lightPos;
uniform vec4 lightColor;

void main(){
    vec3 lightDir = normalize(vec3(1.0, 2.0, 1.0));
    float diff = max(dot(normalize(vNormal), lightDir), 0.0);
    vec3 dirt = vec3(0.55, 0.40, 0.25);
    FragColor = vec4(dirt * vAO * (0.3 + 0.7 * diff) * lightColor.rgb, 1.0);
}