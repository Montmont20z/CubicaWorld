#version 330 core
out vec4 FragColor; // declare the output color variable. Written into framebuffer for each fragment/pixel
void main()
{
    FragColor = vec4(0.8f, 0.3f, 0.00f,1.0f);// Sets the output color to an RGBA value
}