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
    GLuint ID;
    explicit VBO(const std::vector<Vertex> &vertices, GLenum usage);
    ~VBO() noexcept;

    void Bind() const;
    void Unbind() const;
};