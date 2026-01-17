#pragma once
#include <vector>
#include <memory>
#include <glad/glad.h>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

#include "Shader.hpp"
#include "Texture.hpp"
#include "VAO.hpp"
#include "EBO.hpp"
#include "Camera.hpp"

class Mesh {
public:
    Mesh(const std::vector<Vertex> &vertices, const std::vector<GLuint> &indices, std::vector<std::unique_ptr<Texture>>&& textures, GLenum usage);
    ~Mesh() noexcept;

    // no copy 
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh& ) = delete;
    // moveable
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;


    void Draw(const Shader &shader, const Camera &camare) const;
    void SetTexture(const Texture& texture);
    
    

private:
    VAO vao_;
    VBO vbo_;
    EBO ebo_;
    std::vector<std::unique_ptr<Texture>> textures_;
    std::vector<Vertex> vertices_;
    std::vector<GLuint> indices_;

};