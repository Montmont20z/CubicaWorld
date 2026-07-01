#pragma once
#include <memory>
#include <glad/glad.h>
#include "VAO.hpp"
#include "VBO.hpp"
#include "EBO.hpp"
#include "ChunkVertex.hpp"
#include "MeshData.hpp"

// GPU-only object for chunk geometry. Unlike Mesh, owns no textures —
// the shared texture array is bound once by the caller before drawing chunks.
// Main thread only.
class ChunkMesh {
public:
    ChunkMesh() = default;

    ChunkMesh(const ChunkMesh&) = delete;
    ChunkMesh& operator=(const ChunkMesh&) = delete;
    ChunkMesh(ChunkMesh&&) = default;
    ChunkMesh& operator=(ChunkMesh&&) = default;

    // First call allocates VBO/EBO and links VAO attributes once.
    // Later calls reuse the same buffers via VBO::Update/EBO::Update — no VAO rebuild.
    void Upload(MeshData&& data);

    void Draw() const; // shader/camera already set up by caller (ChunkManager)

    GLuint IndexCount() const { return indexCount_; }

private:
    VAO vao_;
    std::unique_ptr<VBO<ChunkVertex>> vbo_; // null until first Upload
    std::unique_ptr<EBO>              ebo_; // null until first Upload
    GLuint indexCount_{0};
};