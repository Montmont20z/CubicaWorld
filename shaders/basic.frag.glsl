#version 330 core
out vec4 FragColor; // declare the output color variable. Written into framebuffer for each fragment/pixel

in vec3 currentPosition;
in vec3 normal;
in vec3 color;
in vec2 texCoord;

uniform sampler2D tex0;

void main()
{
    // FragColor = vec4(color, 1.0f);// Sets the output color to an RGBA value
    FragColor = texture(tex0, texCoord);
}