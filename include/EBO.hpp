#pragma once
#include <glad/glad.h>
#include <vector>

class EBO {
public:
    GLuint ID;
    explicit EBO(const std::vector<GLuint> &indices, GLenum usage);
    ~EBO() noexcept;
    
    // no copy
    EBO(const EBO&) = delete;
    EBO& operator=(const EBO& ) = delete;

    // moveble
    EBO(EBO&& other) noexcept;
    EBO& operator=(EBO&& other) noexcept;
         

    void Bind() const;
    void Unbind() const;
};