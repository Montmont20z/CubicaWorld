#include "VAO.hpp"

VAO::VAO()
{
    glGenVertexArrays(1, &ID);
}

VAO::~VAO() noexcept
{
    glDeleteVertexArrays(1, &ID);
}


void VAO::LinkAttrib(const VBO &VBO, GLuint layout, GLuint numComponent, GLenum type, GLsizeiptr stride, void *offset) const
{
    VBO.Bind();
    // tells OpenGL how to read the vertex data via VAO
    // This line records inside the VAO how attribute location 0 should fetch its data from a VBO.
    glVertexAttribPointer(
        layout,             // attribute location in shader //eg: layout (location = 0) in vec3 aPos;
        numComponent,       // how many item in this attribute (position, normal, uv)? [x,y,z, nx, ny, nz, u, v] = 3,3,2 and [x,y,z,r,g,b] = 3,3 // max attribute is 4
        type,               // data type of each value (float?, int?, uint8_t?)
        GL_FALSE,           // normalize? Convert integer → float? only relavance for int
        stride,             // Distance between vertices (in bytes) - if [float x, y, z] then stride = 3 * sizeof(float); if [float x,y,z, r,g,b] then stride = 6 * sizeof(float)
        offset              // byte offset, not an index. - if [float x,y,z, r,g,b] then first offset, offset = (void*)0; - second offset, offset = (void*)(3 * sizeof(float));
    );

    glEnableVertexAttribArray(layout); // Enables the attribute so the GPU will fetch it for each vertex when drawing.
    VBO.Unbind();
}

void VAO::Bind() const
{
    glBindVertexArray(ID); // Bind the VAO. A VAO stores vertex attribute state (which VBOs to use, [x,y,z, nx, ny, nz, u, v, ...] ).
}

void VAO::Unbind() const
{
    glBindVertexArray(0); // unbind the VAO.
}
