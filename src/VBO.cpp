#include "VBO.hpp"
#include <iostream>

VBO::VBO(const std::vector<Vertex> &vertices, GLenum usage = GL_STATIC_DRAW)
    : byteSize_(0)
    , usage_(usage)
{
    if (vertices.empty()){
        return;
    }
    byteSize_ = vertices.size() * sizeof(Vertex);

    glGenBuffers(1, &ID); 
    if (ID== 0) {
        throw std::runtime_error("VBO: glGenBuffers returned 0\n");
        return;
    }
    // upload vertex data (to GPU)
    glBindBuffer(GL_ARRAY_BUFFER, ID); // Bind the buffer as the current array buffer. Subsequent glBufferData/glVertexAttribPointer calls refer to this buffer.
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(byteSize_), vertices.data(), usage); // GL_STATIC_DRAW hints that data will not change frequenctly
    // unbind VBO
    glBindBuffer(GL_ARRAY_BUFFER, 0); 
}

VBO::~VBO() noexcept
{
    if (ID != 0){
        glDeleteBuffers(1, &ID);
        ID = 0;
    }
}

VBO::VBO(VBO &&other) noexcept
    : ID(other.ID)
    , byteSize_(other.byteSize_)
    , usage_(other.usage_)
{
    other.ID = 0;
    other.byteSize_ = 0;
    other.usage_ = 0;
}

VBO &VBO::operator=(VBO &&other) noexcept
{
    if (this != &other){
        if (this->ID != 0){
           glDeleteBuffers(1, &ID);
        }
        // steal other properties
        this->ID = other.ID;
        this->byteSize_ = other.byteSize_;
        this->usage_ = other.usage_;
        
        // clean other 
        other.ID = 0;
        other.byteSize_ = 0;
        other.usage_ = 0;
    }
    return *this;
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
