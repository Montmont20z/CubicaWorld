#include "Meshing/BlockSampler.hpp"

namespace meshing {

BlockSampler::BlockSampler(const Chunk& chunk, const ChunkNeighbors& neighbors)
    : chunk_(chunk)
    , nx_(neighbors.nx.get())
    , px_(neighbors.px.get())
    , nz_(neighbors.nz.get())
    , pz_(neighbors.pz.get())
{
}


bool BlockSampler::IsAir(int x, int y, int z) const
{
        // Out of vertical bounds -> always air (above) or always solid (below world)
        if (y < 0 || y >= CHUNK_HEIGHT) return y >= CHUNK_HEIGHT;
        
        // Within this chunk
        if (x >= 0 && x < CHUNK_SIZE && z >= 0 && z < CHUNK_SIZE)
            return chunk_.GetBlock(x, y, z) == BlockType::Air;
        
        // Pick which chunk owns this coordinate and wrap into its local space.
        // Only one axis can be out of range at a time (we only ever probe +-1 on one axis).
        const Chunk* chunk = &chunk_;
        if (x < 0)                { chunk = nx_; x = x + CHUNK_SIZE; }
        else if (x >= CHUNK_SIZE) { chunk = px_; x = x - CHUNK_SIZE; }
        else if (z < 0)           { chunk = nz_; z = z + CHUNK_SIZE; }
        else if (z >= CHUNK_SIZE) { chunk = pz_; z = z - CHUNK_SIZE; }

        if (!chunk) return false; // treat as solid if neighbor not loaded
        if (chunk->GetChunkState() == ChunkState::Empty) // avoid reading neighbor chunk during initial generation
            return false;
        return chunk->GetBlock(x, y, z) == BlockType::Air;
    
}

}// namespace meshing