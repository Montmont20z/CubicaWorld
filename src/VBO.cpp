#include "VBO.hpp"

VBO::VBO(const std::vector<Vertex> &vertices, GLenum usage = GL_STATIC_DRAW)
{
    glGenBuffers(1, &ID); 
    // upload vertex data (to GPU)
    glBindBuffer(GL_ARRAY_BUFFER, ID); // Bind the buffer as the current array buffer. Subsequent glBufferData/glVertexAttribPointer calls refer to this buffer.
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), usage); // GL_STATIC_DRAW hints that data will not change frequenctly
}

VBO::~VBO() noexcept
{
    glDeleteBuffers(1, &ID);
}

void VBO::Bind() const
{
    // upload vertex data (to GPU)
    glBindBuffer(GL_ARRAY_BUFFER, ID); // Bind the buffer as the current array buffer. Subsequent glBufferData/glVertexAttribPointer calls refer to this buffer.
}

void VBO::Unbind() const
{
    // unbind VBO
    glBindBuffer(GL_ARRAY_BUFFER, 0); 
}
