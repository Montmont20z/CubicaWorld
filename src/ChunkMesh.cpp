#include "ChunkMesh.hpp"
#include "EBO.hpp"
#include "MeshData.hpp"
#include "PerformanceMetrics.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>

struct ChunkMesh::GpuMemoryLease {
    explicit GpuMemoryLease(std::uint64_t byteCount) : bytes(byteCount) {
        const auto safeBytes = static_cast<std::int64_t>(
            bytes > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())
                ? std::numeric_limits<std::int64_t>::max() : bytes);
        PerformanceMetrics::Instance().AdjustResidentGpuBytes(safeBytes);
    }

    ~GpuMemoryLease() {
        const auto safeBytes = static_cast<std::int64_t>(
            bytes > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())
                ? std::numeric_limits<std::int64_t>::max() : bytes);
        PerformanceMetrics::Instance().AdjustResidentGpuBytes(-safeBytes);
    }

    std::uint64_t bytes = 0;
};

std::uint64_t ChunkMesh::BufferBytes() const noexcept {
    return gpuMemoryLease_ ? gpuMemoryLease_->bytes : 0;
}

void ChunkMesh::Upload(MeshData&& data) {
    const auto newIndexCount = static_cast<GLuint>(data.indices.size());
    const auto newVertexCount = static_cast<GLuint>(data.vertices.size());

    if (newIndexCount == 0 || newVertexCount == 0) {
        indexCount_ = 0;
        vertexCount_ = 0;
        vbo_.reset();
        ebo_.reset();
        gpuMemoryLease_.reset();
        data.Clear();
        return;
    }

    if (!vbo_) {
        // First upload: create buffers and link vertex attributes once.
        vbo_ = std::make_unique<VBO<ChunkVertex>>(data.vertices, GL_DYNAMIC_DRAW);
        ebo_ = std::make_unique<EBO>(data.indices, GL_DYNAMIC_DRAW);

        vao_.Bind();
        ebo_->Bind();
        vao_.LinkAttrib(vbo_->ID, 0, 3, GL_FLOAT, sizeof(ChunkVertex), (void*)offsetof(ChunkVertex, position));
        vao_.LinkAttrib(vbo_->ID, 1, 3, GL_FLOAT, sizeof(ChunkVertex), (void*)offsetof(ChunkVertex, normal));
        vao_.LinkAttrib(vbo_->ID, 2, 2, GL_FLOAT, sizeof(ChunkVertex), (void*)offsetof(ChunkVertex, texCoords));
        vao_.LinkAttrib(vbo_->ID, 3, 1, GL_UNSIGNED_BYTE, sizeof(ChunkVertex), (void*)offsetof(ChunkVertex, texLayer), true);
        vao_.LinkAttrib(vbo_->ID, 4, 1, GL_UNSIGNED_BYTE, sizeof(ChunkVertex), (void*)offsetof(ChunkVertex, ao), true);
        vao_.Unbind();
    } else {
        // Remesh path: reuse VBO/EBO and VAO objects.
        vbo_->Update(data.vertices);
        ebo_->Update(data.indices);
    }

    indexCount_ = newIndexCount;
    vertexCount_ = newVertexCount;

    using IndexType = decltype(data.indices)::value_type;
    const std::uint64_t byteCount =
        static_cast<std::uint64_t>(data.vertices.size() * sizeof(ChunkVertex)) +
        static_cast<std::uint64_t>(data.indices.size() * sizeof(IndexType));

    // Replacing this shared token first releases the previous allocation's
    // accounting, then records the new VBO + EBO payload size.
    gpuMemoryLease_.reset();
    gpuMemoryLease_ = std::make_shared<GpuMemoryLease>(byteCount);

    data.Clear();
}

void ChunkMesh::Draw() const {
    if (indexCount_ == 0) return;

    vao_.Bind();
    glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);

    // This is counted only when an indexed draw is actually submitted.
    PerformanceMetrics::Instance().RecordDrawStatistics(
        1, static_cast<std::uint64_t>(indexCount_) / 3);
}