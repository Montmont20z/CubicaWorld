#include "EBO.hpp"

EBO::EBO(const std::vector<GLuint>& indices, GLenum usage = GL_STATIC_DRAW)
{
    glGenBuffers(1, &ID); 
    // upload index data — (EBO binding is stored in the VAO) - (to GPU)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID); // Bind the buffer as the current array buffer. Subsequent glBufferData/glVertexAttribPointer calls refer to this buffer.
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), usage); // GL_STATIC_DRAW hints that data will not change frequenctly
}

EBO::~EBO() noexcept
{
    glDeleteBuffers(1, &ID);
}

void EBO::Bind() const
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID); 
}

void EBO::Unbind() const
{
    // unbind EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); 
}
