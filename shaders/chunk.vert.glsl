#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in uint aTexLayer;
layout(location = 4) in uint aAO;

uniform mat4 camMatrix;
uniform mat4 model;

out vec3 vNormal;
out float vAO;

void main(){
    gl_Position = camMatrix * model * vec4(aPos, 1.0);
    vNormal = mat3(transpose(inverse(model))) * aNormal;
    vAO = 1.0 - float(aAO) * 0.15;// ao=3 → fully bright, ao=0 → darkest
}