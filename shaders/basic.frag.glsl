#version 330 core
out vec4 FragColor; // declare the output color variable. Written into framebuffer for each fragment/pixel

in vec3 currentPosition;
in vec3 normal;
in vec3 color;
in vec2 texCoord;

uniform sampler2D tex0;
uniform vec4 lightColor;
uniform vec3 lightPos;
uniform vec3 camPos;

void main()
{
    // ambient light
    float ambient = 0.20f;
    
    // diffuse light
    vec3 normal = normalize(normal);
    vec3 lightDirection = normalize(lightPos - currentPosition);
    float diffuse = max(dot(normal, lightDirection), 0.0f);

    // specular light
    float specularLight = 0.5f;
    vec3 viewDirection = normalize(camPos - currentPosition);
    vec3 reflectionDirection = reflect(-lightDirection, normal);
    float specAmount = pow(max(dot(viewDirection, reflectionDirection), 0.0f), 8);
    float specular = specAmount * specularLight;

    // FragColor = texture(tex0, texCoord) * lightColor * (diffuse + ambient + specular);
    FragColor =  vec4(color, 1.0) * lightColor * diffuse ;

}