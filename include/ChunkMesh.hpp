#pragma once
#include <cstdint>
#include <memory>
#include <glad/glad.h>
#include "VAO.hpp"
#include "VBO.hpp"
#include "EBO.hpp"
#include "ChunkVertex.hpp"
#include "MeshData.hpp"

// GPU-only object for chunk geometry. Unlike Mesh, owns no textures —
// the shared texture array is bound once by the caller before drawing chunks.
// Upload / Draw / destruction must occur on the OpenGL context thread.
class ChunkMesh {
public:
    ChunkMesh() = default;

    ChunkMesh(const ChunkMesh&) = delete;
    ChunkMesh& operator=(const ChunkMesh&) = delete;
    ChunkMesh(ChunkMesh&&) = default;
    ChunkMesh& operator=(ChunkMesh&&) = default;

    // First call allocates VBO/EBO and links VAO attributes once.
    // Later calls reuse the same buffers via VBO::Update/EBO::Update.
    void Upload(MeshData&& data);
    void Draw() const;

    GLuint IndexCount() const noexcept { return indexCount_; }
    GLuint VertexCount() const noexcept { return vertexCount_; }
    std::uint64_t BufferBytes() const noexcept;

private:
    // Shared ownership makes the accounting token safe with ChunkMesh's
    // default move operations. The token adjusts the global byte counter
    // exactly once when the underlying chunk mesh allocation is replaced
    // or destroyed.
    struct GpuMemoryLease;

    VAO vao_;
    std::unique_ptr<VBO<ChunkVertex>> vbo_;
    std::unique_ptr<EBO> ebo_;
    std::shared_ptr<GpuMemoryLease> gpuMemoryLease_;
    GLuint indexCount_{0};
    GLuint vertexCount_{0};
};