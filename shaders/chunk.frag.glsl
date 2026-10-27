#version 430 core
in vec3 vNormal;
in float vAO;
in vec2 vTexCoord;
flat in uint vTexLayer;

uniform sampler2D tex0;
uniform int atlasTiles;

uniform vec3 lightPos;
uniform vec4 lightColor;

out vec4 FragColor;

void main(){
    // atlas lookup: layer = tile index
    uint row = (uint(atlasTiles) - 1u) - (vTexLayer / uint(atlasTiles));
    uint col = vTexLayer % uint(atlasTiles);
    // uint row = vTexLayer / uint(atlasTiles);
    vec2 uv = (vec2(col, row) + vTexCoord) / float(atlasTiles);
    
    vec3 albedo = texture(tex0, uv).rgb;

    vec3 lightDir = normalize(vec3(1.0, 2.0, 1.0));
    float diff = max(dot(normalize(vNormal), lightDir), 0.0);
    FragColor = vec4(albedo * vAO * (0.3 + 0.7 * diff) * lightColor.rgb, 1.0);
}