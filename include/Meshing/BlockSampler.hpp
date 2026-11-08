// BlockSampler.hpp
#pragma once

#include "BlockType.hpp"
#include "Chunk.hpp"   // Chunk + ChunkNeighbors

namespace meshing {

// Read-only view of a chunk plus its 4 horizontal neighbours.
// Cheap to construct, lives on the stack for the duration of one BuildMesh call.
class BlockSampler {
public:
    BlockSampler(const Chunk& chunk, const ChunkNeighbors& neighbors);

    // Block inside this chunk (bounds-checked, Air if out of range).
    BlockType Get(int x, int y, int z) const { return chunk_.GetBlock(x, y, z); }

    // True if the block at (x, y, z) is air. Handles:
    //  - above the world  -> air
    //  - below the world  -> solid
    //  - x/z one block outside the chunk -> asks the neighbour
    //  - neighbour missing / still Empty -> treated as SOLID so we don't emit
    //    faces that would pop in once the neighbour finishes loading
    bool IsAir(int x, int y, int z) const;

private:
    const Chunk& chunk_;
    // Raw pointers on purpose: copying shared_ptr in a hot loop costs an
    // atomic refcount inc/dec per call. The ChunkNeighbors struct outlives us.
    const Chunk* nx_;
    const Chunk* px_;
    const Chunk* nz_;
    const Chunk* pz_;
};

} // namespace meshing