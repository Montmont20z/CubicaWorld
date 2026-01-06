#include "EBO.hpp"

EBO::EBO(GLuint *indices, GLsizeiptr size)
{
    glGenBuffers(1, &ID); 
    // upload index data — (EBO binding is stored in the VAO) - (to GPU)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID); // Bind the buffer as the current array buffer. Subsequent glBufferData/glVertexAttribPointer calls refer to this buffer.
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices, GL_STATIC_DRAW); // GL_STATIC_DRAW hints that data will not change frequenctly
}

EBO::~EBO() noexcept
{
    glDeleteBuffers(1, &ID);
}

void EBO::Bind() const
{
    // upload vertex data (to GPU)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID); // Bind the buffer as the current array buffer. Subsequent glBufferData/glVertexAttribPointer calls refer to this buffer.
}

void EBO::Unbind() const
{
    // unbind EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); 
}
