#version 330 core
out vec4 FragColor; // declare the output color variable. Written into framebuffer for each fragment/pixel

in vec3 currentPosition; // Fragment's world position
in vec3 normal;
in vec3 color; // // Vertex color
in vec2 texCoord;

uniform sampler2D tex0;
uniform vec4 lightColor;
uniform vec3 lightPos;
uniform vec3 camPos;

vec4 pointLight(){
    // implement inverse square law
    vec3 lightVector = lightPos - currentPosition; 
    float distance = length(lightVector);
    float a = 1.0;
    float b = 0.7;
    float intensity = 1.0f / (a * distance * distance + b * distance + 1.0f);

   // ambient light
    float ambient = 0.04f;
    
    // diffuse light
    vec3 n = normalize(normal);
    vec3 lightDirection = normalize(lightVector);
    float diffuse = max(dot(n, lightDirection), 0.0f); //  min is 0.0f

    // specular light
    float specularLight = 0.5f; // intensity of shinyness
    vec3 viewDirection = normalize(camPos - currentPosition);
    vec3 reflectionDirection = reflect(-lightDirection, n);
    float specAmount = pow(max(dot(viewDirection, reflectionDirection), 0.0f), 8); // pow() shininess exponent. Higher: shaper highlight (eg metal)
    float specular = specAmount * specularLight;

    return texture(tex0, texCoord) * lightColor * (diffuse * intensity + ambient + specular * intensity);
    // return vec4(color, 1.0) * lightColor * (diffuse + ambient + specular); 
}

vec4 directionalLight(){

   // ambient light
    float ambient = 0.04f;
    
    // diffuse light
    vec3 n = normalize(normal);
    vec3 lightDirection = normalize(vec3(-1.0f, 0.0f, 0.0f));
    float diffuse = max(dot(n, lightDirection), 0.0f); //  min is 0.0f

    // specular light
    float specularLight = 0.5f; // intensity of shinyness
    vec3 viewDirection = normalize(camPos - currentPosition);
    vec3 reflectionDirection = reflect(-lightDirection, n);
    float specAmount = pow(max(dot(viewDirection, reflectionDirection), 0.0f), 8); // pow() shininess exponent. Higher: shaper highlight (eg metal)
    float specular = specAmount * specularLight;

    return texture(tex0, texCoord) * lightColor * (diffuse + ambient + specular );
}

vec4 spotLight(){
    float outerCone = 0.95f;
    float innerCone = 0.9f;

   // ambient light
    float ambient = 0.04f;
    
    // diffuse light
    vec3 n = normalize(normal);
    vec3 lightDirection = normalize(lightPos - currentPosition);
    float diffuse = max(dot(n, lightDirection), 0.0f); //  min is 0.0f

    // specular light
    float specularLight = 0.5f; // intensity of shinyness
    vec3 viewDirection = normalize(camPos - currentPosition);
    vec3 reflectionDirection = reflect(-lightDirection, n);
    float specAmount = pow(max(dot(viewDirection, reflectionDirection), 0.0f), 8); // pow() shininess exponent. Higher: shaper highlight (eg metal)
    float specular = specAmount * specularLight;
    
    float angle = dot(vec3(0.0f, -1.0f, 0.0f), -lightDirection);
    float intensity = clamp((angle - outerCone)/(innerCone - outerCone), 0.0f, 1.0f);

    return texture(tex0, texCoord) * lightColor * (diffuse*intensity + ambient + specular*intensity );
}


void main()
{
    FragColor = spotLight();
    // FragColor =  vec4(color, 1.0) * lightColor * (diffuse + ambient + specular);

}