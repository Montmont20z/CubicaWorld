#pragma once
#include <vector>
#include <glad/glad.h>
#include "ChunkVertex.hpp"

// Pure CPU data - no GL calls, safe to build on any threads 
// Matches the packed vertex format for chunk geometry
struct MeshData
{
    std::vector<ChunkVertex> vertices;
    std::vector<GLuint> indices;
    bool Empty() const { return indices.empty(); }
    void Clear() { vertices.clear(); indices.clear(); }
};
