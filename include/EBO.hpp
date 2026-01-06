#pragma once
#include <glad/glad.h>

class EBO {
public:
    GLuint ID;
    explicit EBO(GLuint* indices, GLsizeiptr size);
    ~EBO() noexcept;
    
    void Bind() const;
    void Unbind() const;
};