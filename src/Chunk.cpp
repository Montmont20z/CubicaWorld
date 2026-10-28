#include "Chunk.hpp"
#include "BlockTexture.hpp"
#include "BlockType.hpp"
#include "Constants.hpp"
#include "MeshData.hpp"
#include "ChunkMesh.hpp"
#include "RenderSettings.hpp"

#include <atomic>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <glm/fwd.hpp>
#include <glm/gtx/transform.hpp>
#include <memory>
#include <vector>

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
    : coord_(coord)
    , blocks_(std::make_unique<BlockType[]>(CHUNK_SIZE*CHUNK_HEIGHT*CHUNK_SIZE))
{
}

void Chunk::Generate(){
    // Flat terrain placeholder - swap in Perlin noise later.
    // Everything below y=64 is Dirt; above is Air.
    for (int x = 0; x < CHUNK_SIZE; ++x) {
        for (int y = 0; y < CHUNK_HEIGHT; ++y) {
            for (int z = 0; z < CHUNK_SIZE; ++z) {
                // BlockAt(x,y,z) = BlockType::Air;
                BlockAt(x,y,z) = (y <= SURFACE_Y) ? BlockType::Dirt : BlockType::Air;
            }
        }
    }

    for (int x = 0; x < CHUNK_SIZE; ++x) {
        for (int y = 0; y < CHUNK_HEIGHT; ++y) {
            for (int z = 0; z < CHUNK_SIZE; ++z) {
                // int surfaceY = static_cast<int>(std::sin(x * 0.2f) * 5.0f) + SURFACE_Y;
                // BlockAt(x,surfaceY, z) = BlockType::Stone;
            }
        }
    }
    // BlockAt(0, SURFACE_Y, 0) = BlockType::Sand;
    // BlockAt(0, 70, 14) = BlockType::Grass;
    // BlockAt(0, 70, 1) = BlockType::Stone;
    // BlockAt(1, 64, 0) = BlockType::Stone;
    // BlockAt(2, 65, 0) = BlockType::Log;
    // BlockAt(3, 64, 0) = BlockType::Leaf;
    state_.store(ChunkState::Generated, std::memory_order_release); 
}


void Chunk::BuildMesh(const ChunkNeighbors& neighbors){
    
    assert(State() == ChunkState::Generated || State() == ChunkState::Dirty);

    // Return true if the given world-local coord is air (or out of bounds or unloaded neighbor)
    // For cross-chunk border queires, falls back to the neighbor pointer.
    // If neighbor is null (not loaded), we treat the block as SOLID to avoid emitting
    // faces that would pop in when the neighbor loads. This is conservative but correct.
    auto isAir = [&](int x, int y, int z) -> bool {
        // Out of vertical bounds -> always air (above) or always solid (below world)
        if (y < 0 || y >= CHUNK_HEIGHT) return y >= CHUNK_HEIGHT;
        
        // Within this chunk
        if (x >= 0 && x < CHUNK_SIZE && z >= 0 && z < CHUNK_SIZE)
            return BlockAt(x, y, z) == BlockType::Air;
        
        // Cross-chunk border - query neighbor
        std::shared_ptr<const Chunk> neighbor = nullptr;
        int nx = x, nz = z;
        if (x < 0)                { neighbor = neighbors.nx; nx = x + CHUNK_SIZE; }
        else if (x >= CHUNK_SIZE) { neighbor = neighbors.px; nx = x - CHUNK_SIZE; }
        else if (z < 0)           { neighbor = neighbors.nz; nz = z + CHUNK_SIZE; }
        else if (z >= CHUNK_SIZE) { neighbor = neighbors.pz; nz = z - CHUNK_SIZE; }

        if (!neighbor) return false; // treat as solid if neigbor not loaded
        if (neighbor->State() == ChunkState::Empty) // avoid reading neighbor chunk during initial generation
            return false;
        return neighbor->GetBlock(nx, y, nz) == BlockType::Air;
    };
    
    auto appendFace = [](const ChunkVertex face[4],uint8_t tile, int bx, int by, int bz,
                                    std::vector<ChunkVertex>& verts, std::vector<GLuint>& idxs)
    {
        const auto base = static_cast<GLuint>(verts.size());     
        const glm::vec3 offset(static_cast<float>(bx),
                                static_cast<float>(by),
                                static_cast<float>(bz));

        for (int i = 0; i < 4; ++i) {
            ChunkVertex v = face[i];
            v.position += offset;
            v.texLayer = tile; // numbers for specific blockType
            verts.push_back(v);
        }
        // Two CCW triangles: 0-1-2, 2-3-0
        idxs.push_back(base + 0); idxs.push_back(base + 1); idxs.push_back(base + 2);
        idxs.push_back(base + 2); idxs.push_back(base + 3); idxs.push_back(base + 0);
    };
    
    MeshData data;
    data.vertices.reserve(4096);
    data.indices.reserve(6144);
    // Temporary hardcoded tiles (index = row*16 + col, see note below)
    constexpr uint8_t TILE_TOP = 0, TILE_SIDE = 1, TILE_BOTTOM = 2;
    
    for (int x = 0; x < CHUNK_SIZE; ++x) {
        for (int y = 0; y < CHUNK_HEIGHT; ++y) {
            for (int z = 0; z < CHUNK_SIZE; ++z) {
                const BlockType b = BlockAt(x,y,z);
                if (b == BlockType::Air) continue;
                // Add face to mesh
                if (FACE_CULLING){
                    if (isAir(x+1, y, z)) appendFace(RIGHT_FACE, TileFor(b, Face::Right),  x,y,z, data.vertices, data.indices);
                    if (isAir(x-1, y, z)) appendFace(LEFT_FACE, TileFor(b, Face::Left), x, y, z, data.vertices, data.indices);
                    if (isAir(x, y+1, z)) appendFace(TOP_FACE,TileFor(b, Face::Top), x, y, z, data.vertices, data.indices);
                    if (isAir(x, y-1, z)) appendFace(BOTTOM_FACE, TileFor(b, Face::Bottom), x, y, z, data.vertices, data.indices);
                    if (isAir(x, y, z+1)) appendFace(FRONT_FACE,TileFor(b, Face::Front),  x, y, z, data.vertices, data.indices);
                    if (isAir(x, y, z-1)) appendFace(BACK_FACE, TileFor(b, Face::Back), x, y, z, data.vertices, data.indices);
                }
                else { // Without face culling
                    appendFace(RIGHT_FACE, TileFor(b, Face::Right),  x,y,z, data.vertices, data.indices);
                    appendFace(LEFT_FACE, TileFor(b, Face::Left), x, y, z, data.vertices, data.indices);
                    appendFace(TOP_FACE,TileFor(b, Face::Top), x, y, z, data.vertices, data.indices);
                    appendFace(BOTTOM_FACE, TileFor(b, Face::Bottom), x, y, z, data.vertices, data.indices);
                    appendFace(FRONT_FACE,TileFor(b, Face::Front),  x, y, z, data.vertices, data.indices);
                    appendFace(BACK_FACE, TileFor(b, Face::Back), x, y, z, data.vertices, data.indices);
                }

            }
        }
    
    }
    pendingMesh_ = std::move(data);
    auto expected = ChunkState::Generated;
    state_.compare_exchange_strong(expected, ChunkState::MeshReady, std::memory_order_acq_rel); // If it fails, someone set Dirty mid-build. Leave it alone; next frame remeshes.
}

 
// ---------------------------------------------------------------------------
// Main thread only from here down.

void Chunk::UploadIfReady(){
    if (State() != ChunkState::MeshReady) return;
    chunkMesh_.Upload(std::move(pendingMesh_));    // Upload() clears pendingMesh_ internally
    state_.store(ChunkState::Uploaded, std::memory_order_release);
}

void Chunk::Render(const Shader& shader, const Camera& camera) const
{
   if (State() != ChunkState::Uploaded) return; 
   if (chunkMesh_.IndexCount() == 0) return;
   
   // World position derived from coord - no extra parameters.
   const glm::vec3 worldPos(
        static_cast<float>(coord_.x) * CHUNK_SIZE,
        0.0f,
        static_cast<float>(coord_.z) * CHUNK_SIZE
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
