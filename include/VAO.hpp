#pragma once
#include <glad/glad.h>
#include "VBO.hpp"

class VAO {
public:
    GLuint ID;
    explicit VAO();
    ~VAO() noexcept;
    
    void LinkAttrib(const VBO& VBO, GLuint layout, GLuint numComponent, GLenum type, GLsizeiptr stride, void* offset) const;
    void Bind() const;
    void Unbind() const;
};