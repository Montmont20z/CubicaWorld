#version 330 core
out vec4 FragColor; // declare the output color variable. Written into framebuffer for each fragment/pixel

in vec3 color;

void main()
{
    FragColor = vec4(color, 1.0f);// Sets the output color to an RGBA value
}