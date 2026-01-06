#pragma once
#include <glad/glad.h>
#include "VBO.hpp"

class VAO {
public:
    GLuint ID;
    explicit VAO();
    ~VAO() noexcept;
    
    void LinkVBO(const VBO& VBO, GLuint layout) const;
    void Bind() const;
    void Unbind() const;
};