#pragma once
#include <glad/glad.h>
#include <vector>

class EBO {
public:
    GLuint ID{0};
    GLsizei Count{0};

    explicit EBO(const std::vector<GLuint> &indices, GLenum usage);
    ~EBO() noexcept;
    
    // no copy
    EBO(const EBO&) = delete;
    EBO& operator=(const EBO& ) = delete;

    // moveble
    EBO(EBO&& other) noexcept;
    EBO& operator=(EBO&& other) noexcept;
         
    void Update(const std::vector<GLuint>& indices);
    void Bind() const;
    void Unbind() const;
private:
    GLenum usage_{GL_STATIC_DRAW};
};