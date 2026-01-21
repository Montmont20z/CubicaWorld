#pragma once
#include <glad/glad.h>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <vector>

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;
    glm::vec2 texCoords;
};

class VBO {
public:
    GLuint ID{0};
    explicit VBO(const std::vector<Vertex> &vertices, GLenum usage);
    ~VBO() noexcept;
    
    // Delete Copy
    VBO(const VBO&) = delete;
    VBO& operator=(const VBO&) = delete;

    // Move
    VBO(VBO&& other) noexcept;
    VBO& operator=(VBO&& other) noexcept;

    void Bind() const;
    void Unbind() const;
private:
    std::size_t byteSize_{0};
    GLenum usage_{GL_STATIC_DRAW};
};