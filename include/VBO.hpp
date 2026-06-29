#pragma once
#include <glad/glad.h>
#include <vector>
#include "Vertex.hpp"

// Templated because the buffer logic (gen/bind/upload/delete) is identical
// for any vertex type — this is the real DRY case, unlike Mesh/ChunkMesh
// which differ in attribute layout, not buffer logic.
// template<typename VertexT>
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