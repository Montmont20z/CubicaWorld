#include "Chunk.hpp"
#include "BlockType.hpp"
#include "Constants.hpp"
#include "Logger.hpp"
#include "MeshData.hpp"
#include "ChunkMesh.hpp"
#include "RenderSettings.hpp"
#include "TerrainGenerator.hpp"
#include "Meshing/BlockSampler.hpp"
#include "Meshing/NaiveMesher.hpp"
#include "Meshing/GreedyMesher.hpp"

#include <atomic>
#include <cassert>
#include <cmath>
#include <glm/fwd.hpp>
#include <glm/gtx/transform.hpp>
#include <memory>
#include <string>
#include <vector>
#include <glm/gtc/noise.hpp>

// Face Template in local scope - no ODR issues, not visible outside this TU
// Position is in block local space (0..1 range), translate per-block in BuildMesh

namespace {
// texLayer=0, ao=3 (full bright) — placeholders until texture array + AO pass exist.
const ChunkVertex RIGHT_FACE[4] = {
    {{ 0.5f, 0.0f, -0.5f}, { 1,0,0}, {0,0}, 0, 3},
    {{ 0.5f, 1.0f, -0.5f}, { 1,0,0}, {0,1}, 0, 3},
    {{ 0.5f, 1.0f,  0.5f}, { 1,0,0}, {1,1}, 0, 3},
    {{ 0.5f, 0.0f,  0.5f}, { 1,0,0}, {1,0}, 0, 3},
};
const ChunkVertex LEFT_FACE[4] = {
    {{-0.5f, 0.0f,  0.5f}, {-1,0,0}, {0,0}, 0, 3},
    {{-0.5f, 1.0f,  0.5f}, {-1,0,0}, {0,1}, 0, 3},
    {{-0.5f, 1.0f, -0.5f}, {-1,0,0}, {1,1}, 0, 3},
    {{-0.5f, 0.0f, -0.5f}, {-1,0,0}, {1,0}, 0, 3},
};
const ChunkVertex TOP_FACE[4] = {
    {{-0.5f, 1.0f,  0.5f}, {0,1,0}, {0,0}, 0, 3},
    {{ 0.5f, 1.0f,  0.5f}, {0,1,0}, {1,0}, 0, 3},
    {{ 0.5f, 1.0f, -0.5f}, {0,1,0}, {1,1}, 0, 3},
    {{-0.5f, 1.0f, -0.5f}, {0,1,0}, {0,1}, 0, 3},
};
const ChunkVertex BOTTOM_FACE[4] = {
    {{-0.5f, 0.0f, -0.5f}, {0,-1,0}, {0,0}, 0, 3},
    {{ 0.5f, 0.0f, -0.5f}, {0,-1,0}, {1,0}, 0, 3},
    {{ 0.5f, 0.0f,  0.5f}, {0,-1,0}, {1,1}, 0, 3},
    {{-0.5f, 0.0f,  0.5f}, {0,-1,0}, {0,1}, 0, 3},
};
const ChunkVertex FRONT_FACE[4] = {
    {{-0.5f, 0.0f,  0.5f}, {0,0,1}, {0,0}, 0, 3},
    {{ 0.5f, 0.0f,  0.5f}, {0,0,1}, {1,0}, 0, 3},
    {{ 0.5f, 1.0f,  0.5f}, {0,0,1}, {1,1}, 0, 3},
    {{-0.5f, 1.0f,  0.5f}, {0,0,1}, {0,1}, 0, 3},
};
const ChunkVertex BACK_FACE[4] = {
    {{ 0.5f, 0.0f, -0.5f}, {0,0,-1}, {0,0}, 0, 3},
    {{-0.5f, 0.0f, -0.5f}, {0,0,-1}, {1,0}, 0, 3},
    {{-0.5f, 1.0f, -0.5f}, {0,0,-1}, {1,1}, 0, 3},
    {{ 0.5f, 1.0f, -0.5f}, {0,0,-1}, {0,1}, 0, 3},
};
} // namespace
  
Chunk::Chunk(ChunkCoord coord)
    : chunkCoord_(coord)
    , blocks_(std::make_unique<BlockType[]>(CHUNK_SIZE*CHUNK_HEIGHT*CHUNK_SIZE))
{
}

void Chunk::Generate(const TerrainGenerator& generator){
    // Render flat terrain 
    // for (int x = 0; x < CHUNK_SIZE; ++x) {
    //     for (int y = 0; y < CHUNK_HEIGHT; ++y) {
    //         for (int z = 0; z < CHUNK_SIZE; ++z) {
    //             BlockAt(x,y,z) = (y <= SURFACE_Y) ? BlockType::Stone: BlockType::Air;
    //         }
    //     }
    // }
    // 
    // 2d Sin wave terrain
    // int surfaceY = static_cast<int>(std::sin(z * 0.2f) * 8.0f) + static_cast<int>(std::sin(x * 0.2f) * 8.0f) + SURFACE_Y;
    // BlockAt(x,surfaceY, z) = BlockType::Grass;

    for (int x = 0; x < CHUNK_SIZE; ++x) {
        for (int z = 0; z < CHUNK_SIZE; ++z) {
            
            const int worldX = chunkCoord_.x * CHUNK_SIZE + x;
            const int worldZ = chunkCoord_.z * CHUNK_SIZE + z;
            
            TerrainColumn column = generator.GenerateColumn(worldX, worldZ);
            
            // Store surface height
            surfaceHeight_[x][z] = column.surfaceY;

            // fill up blocks in Current Chunk with Terrain Column
            for (int y = 0; y < CHUNK_HEIGHT; ++y) {
                BlockAt(x, y, z) = column.blocks[y];
            }
            
        }
    }
    // std::cout << "Perlin: " << glm::perlin(glm::vec2(1.5f,10.0f)) << std::endl;
    state_.store(ChunkState::Generated, std::memory_order_release); 
}


void Chunk::BuildMesh(const ChunkNeighbors& neighbors){
    
    assert(GetChunkState() == ChunkState::Generated || GetChunkState() == ChunkState::Dirty);

    const meshing::BlockSampler sampler(*this, neighbors);
    MeshData data;
    data.vertices.reserve(4096);
    data.indices.reserve(6144);

    switch (MESHING_MODE) {
        case MeshingMode::Naive:       meshing::MeshNaive(sampler, false, data); break;
        case MeshingMode::NaiveCulled: meshing::MeshNaive(sampler, true,  data); break;
        case MeshingMode::Greedy:      meshing::GreedyMesher(sampler).Build(data); break;
    }
    
    pendingMesh_ = std::move(data);
    auto expected = ChunkState::Generated;
    state_.compare_exchange_strong(expected, ChunkState::MeshReady, std::memory_order_acq_rel); // If it fails, someone set Dirty mid-build. Leave it alone; next frame remeshes.
}

 
// ---------------------------------------------------------------------------
// Main thread only from here down.

void Chunk::UploadIfReady(){
    if (GetChunkState() != ChunkState::MeshReady) return;
    chunkMesh_.Upload(std::move(pendingMesh_));    // Upload() clears pendingMesh_ internally
    state_.store(ChunkState::Uploaded, std::memory_order_release);
}

void Chunk::Render(const Shader& shader, const Camera& camera) const
{
   if (GetChunkState() != ChunkState::Uploaded) return; 
   if (chunkMesh_.IndexCount() == 0) return;
   
   // World position derived from coord - no extra parameters.
   const glm::vec3 worldPos(
        static_cast<float>(chunkCoord_.x) * CHUNK_SIZE,
        0.0f,
        static_cast<float>(chunkCoord_.z) * CHUNK_SIZE
   );
   shader.SetMat4("model", glm::translate(glm::mat4(1.0f), worldPos));
   camera.SetUniformMatrix(shader, "camMatrix");
   chunkMesh_.Draw();
}


BlockType Chunk::GetBlock(int x, int y, int z)const {
    if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_SIZE)
        return BlockType::Air;
    return BlockAt(x, y, z);
}

void Chunk::SetBlock(int x, int y, int z, BlockType type) {
    if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_SIZE) return;
    BlockAt(x, y, z) = type;
    MarkDirty();
}

AABB Chunk::GetBoundingBox() const {
    const glm::vec3 minP(chunkCoord_.x * CHUNK_SIZE, 0.0f,chunkCoord_.z * CHUNK_SIZE);
    const glm::vec3 maxP = minP + glm::vec3(CHUNK_SIZE, CHUNK_HEIGHT, CHUNK_SIZE) ;
    
    return {minP, maxP};
};


int Chunk::GetSurfaceHeight(int worldX, int worldZ) const {
    const int localX = worldX - chunkCoord_.x * CHUNK_SIZE;
    const int localZ = worldZ - chunkCoord_.z * CHUNK_SIZE;

    assert(localX >= 0 && localX < CHUNK_SIZE);
    assert(localZ >= 0 && localZ < CHUNK_SIZE);

    return surfaceHeight_[localX][localZ];
}