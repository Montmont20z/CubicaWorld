#include "VAO.hpp"

VAO::VAO()
{
    glGenVertexArrays(1, &ID);
}

VAO::~VAO() noexcept
{
    glDeleteVertexArrays(1, &ID);
}

void VAO::LinkVBO(const VBO& VBO, GLuint layout) const
{
    VBO.Bind();
    // tells OpenGL how to read the vertex data via VAO
    // This line records inside the VAO how attribute location 0 should fetch its data from a VBO.
    glVertexAttribPointer(
        layout,                      // attribute location in shader
        3,                      // number of components
        GL_FLOAT,               // data type
        GL_FALSE,               // normalize?
        3 * sizeof(float),      // stride
        (void*)0                // offset
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
