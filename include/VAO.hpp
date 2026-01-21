#pragma once
#include <glad/glad.h>
#include "VBO.hpp"

class VAO {
public:
    GLuint ID{0};
    explicit VAO();
    ~VAO() noexcept;
    
    // no copy
    VAO(const VAO&) = delete;
    VAO& operator=(const VAO&) = delete;

    // movable 
    VAO(VAO&& other) noexcept;
    VAO& operator=(VAO&& other) noexcept;
    
    // NOTE: caller must bind VAO before calling LinkAttrib so the attribute state is recorded in this VAO. 
    void LinkAttrib(const VBO& VBO, GLuint layout, GLuint numComponent, GLenum type, GLsizeiptr stride, void* offset) const;
    void Bind() const;
    void Unbind() const;
};