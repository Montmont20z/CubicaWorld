#include "EBO.hpp"

EBO::EBO(const std::vector<GLuint>& indices, GLenum usage = GL_STATIC_DRAW)
    : usage_(usage)
{
    Count = static_cast<GLsizei>(indices.size());
    if (indices.empty()){
        ID = 0;
        return;
    }

    glGenBuffers(1, &ID); 
    // upload index data — (EBO binding is stored in the VAO) - (to GPU)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID); // Bind the buffer as the current array buffer. Subsequent glBufferData/glVertexAttribPointer calls refer to this buffer.
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizei>(indices.size() * sizeof(GLuint)), indices.data(), usage_);
}

EBO::~EBO() noexcept
{
    if (ID != 0){
        glDeleteBuffers(1, &ID);
        ID = 0;
    }
}

EBO::EBO(EBO &&other) noexcept
    : ID(other.ID)
    , Count(other.Count)
{
    other.ID = 0;
    other.Count = 0;
}

EBO& EBO::operator=(EBO&& other) noexcept
{
    if (this != &other){
        // free current
        if (ID != 0){
            glDeleteBuffers(1, &ID);
        }
        // steal other properties
        this->ID = other.ID;
        this->Count = other.Count;
        
        // clear other properties
        other.ID = 0;
        other.Count = 0;
    }
    return *this;
}


void EBO::Update(const std::vector<GLuint>& indices)
{
    Count = static_cast<GLsizei>(indices.size());
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, Count * sizeof(GLuint), indices.data(), usage_);
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
