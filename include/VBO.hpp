#pragma once
#include <glad/glad.h>

class VBO {
public:
    GLuint ID;
    explicit VBO(GLfloat* vertices, GLsizeiptr size);
    ~VBO() noexcept;
    
    void Bind() const;
    void Unbind() const;
};