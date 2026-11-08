#include "Chunk.hpp"
#include "BlockTexture.hpp"
#include "BlockType.hpp"
#include "Constants.hpp"
#include "MeshData.hpp"
#include "ChunkMesh.hpp"
#include "RenderSettings.hpp"
#include "TerrainGenerator.hpp"

#include <atomic>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <glm/fwd.hpp>
#include <glm/gtx/transform.hpp>
#include <memory>
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

    // 1. Check whether a block is air.
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
        if (neighbor->GetChunkState() == ChunkState::Empty) // avoid reading neighbor chunk during initial generation
            return false;
        return neighbor->GetBlock(nx, y, nz) == BlockType::Air;
    };
    
    // ------------------------------------------------------------------
    // 2. Return the texture tile of visible face.
    //
    // -1 means there is no face to render
    // 0-255 represents the texture tile
    // ------------------------------------------------------------------
    auto getVisibleTexture = [&](Face face, int slice, int u, int v) -> int {
        int x = 0, y = 0, z = 0;
        int nx = 0, ny = 0, nz = 0;
        
        switch (face){
            case Face::Right:
                x = slice; y = v; z = u;
                
                nx = x + 1;
                ny = y;
                nz = z;
                break;

            case Face::Left:
                x = slice; y = v; z = u;
                
                nx = x - 1;
                ny = y;
                nz = z;
                break;
        
            case Face::Top:
                x = u;
                y = slice;
                z = v;
                
                nx = x;
                ny = y + 1 ;
                nz = z;
                break;

            case Face::Bottom:
                x = u;
                y = slice;
                z = v;
                
                nx = x;
                ny = y - 1 ;
                nz = z;
                break;
            case Face::Front:
                x = u;
                y = v;
                z = slice;
                
                nx = x;
                ny = y;
                nz = z + 1;
                break;

            case Face::Back:
                x = u;
                y = v;
                z = slice;
                
                nx = x;
                ny = y;
                nz = z - 1;
                break;
        }
        const BlockType block = BlockAt(x, y, z);
        
        if (block == BlockType::Air)
            return -1;
        
        // Only emit faces exposed to air.
        if (!isAir(nx, ny, nz))
            return -1;
        
        return static_cast<int>(TileFor(block, face));
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

    // ---------------------------------------------------------
    // 3. Emit one rectangular quad.
    //
    // u and v describe the starting position in the 2D mask.
    // width and height describe how many blocks are merged.
    // ---------------------------------------------------------
    auto emitQuad = [&](Face face, int slice, int u, int v, int width, int height, uint8_t tile){

        glm::vec3 positions[4];
        glm::vec2 uvs[4];

        glm::vec3 normal(0.0f);
        
        const float S = static_cast<float>(slice);
        const float U = static_cast<float>(u);
        const float V = static_cast<float>(v);
        const float W = static_cast<float>(width);
        const float H = static_cast<float>(height);

        switch (face) {
            case Face::Right:  // +X
                normal = {1,0,0};
                
                positions[0] = { S + 0.5f, V    , U - 0.5f};
                positions[1] = { S + 0.5f, V + H, U - 0.5f};
                positions[2] = { S + 0.5f, V + H, U + W - 0.5f};
                positions[3] = { S + 0.5f, V    , U + W - 0.5f};

                uvs[0] = {0,0};
                uvs[1] = {0,H};
                uvs[2] = {W,H};
                uvs[3] = {W,0};
                break;
            case Face::Left:  // -X
                normal = {-1,0,0};
                
                positions[0] = { S - 0.5f, V    , U + W - 0.5f};
                positions[1] = { S - 0.5f, V + H, U + W - 0.5f};
                positions[2] = { S - 0.5f, V + H, U - 0.5f};
                positions[3] = { S - 0.5f, V    , U - 0.5f};

                uvs[0] = {0,0};
                uvs[1] = {0,H};
                uvs[2] = {W,H};
                uvs[3] = {W,0};
                break;
            case Face::Top:  // +Y
                normal = {0,1,0};
                
                positions[0] = { U - 0.5f    , S + 1.0f, V + H - 0.5f};
                positions[1] = { U + W - 0.5f, S + 1.0f, V + H - 0.5f};
                positions[2] = { U + W - 0.5f, S + 1.0f, V - 0.5f};
                positions[3] = { U - 0.5f    , S + 1.0f, V - 0.5f};

                uvs[0] = {0,0};
                uvs[1] = {W,0};
                uvs[2] = {W,H};
                uvs[3] = {0,H};
                break;
            case Face::Bottom:  // -Y
                normal = {0,-1,0};
                
                positions[0] = { U - 0.5f    , S, V - 0.5f};
                positions[1] = { U + W - 0.5f, S, V - 0.5f};
                positions[2] = { U + W - 0.5f, S, V + H - 0.5f};
                positions[3] = { U - 0.5f    , S, V + H - 0.5f};

                uvs[0] = {0,0};
                uvs[1] = {W,0};
                uvs[2] = {W,H};
                uvs[3] = {0,H};
                break;
            case Face::Front:  // +Z
                normal = {0,0,1};
                
                positions[0] = { U - 0.5f,     V,     S + 0.5f };
                positions[1] = { U + W - 0.5f, V,     S + 0.5f };
                positions[2] = { U + W - 0.5f, V + H, S + 0.5f };
                positions[3] = { U - 0.5f,     V + H, S + 0.5f };

                uvs[0] = {0,0};
                uvs[1] = {W,0};
                uvs[2] = {W,H};
                uvs[3] = {0,H};
                break;
            case Face::Back:  // -Z
                normal = {0,0,-1};
                
                positions[0] = { U + W - 0.5f, V + H, S - 0.5f };
                positions[1] = { U - 0.5f    , V + H, S - 0.5f };
                positions[2] = { U - 0.5f    , V    , S - 0.5f };
                positions[3] = { U + W - 0.5f, V    , S - 0.5f };

                uvs[0] = {0,H};
                uvs[1] = {W,H};
                uvs[2] = {W,0};
                uvs[3] = {0,0};
                break;
            default: 
                assert(false && "Invalid Face");
        }

        const GLuint base = static_cast<GLuint>(data.vertices.size());
        
        for (int i = 0; i < 4; ++i){
            data.vertices.push_back({
                positions[i],
                normal,
                uvs[i],
                tile,
                3
            });
        }

        // Two CCW triangles: 0-1-2, 2-3-0
        data.indices.push_back(base + 0); data.indices.push_back(base + 1); data.indices.push_back(base + 2);
        data.indices.push_back(base + 2); data.indices.push_back(base + 3); data.indices.push_back(base + 0);
    };
    
    if (GREEDY_MESHING){
        // ---------------------------------------------------------
        // 4. Greedy meshing.
        // ---------------------------------------------------------
        const Face faces[] = {
            Face::Right,
            Face::Left,
            Face::Top,
            Face::Bottom,
            Face::Front,
            Face::Back
        };
        
        for (Face face: faces){
            int sliceCount = 0;
            int uSize = 0;
            int vSize = 0;
            
            // Decide which axis is fixed and which axes form
            // the 2D mask.
            // max possible size for Right, Left, Front, Back is CHUNK_SIZE * CHUNK_SIZE
            // max possible size for Top, Bottom is CHUNK_SIZE * CHUNK_HEIGHT
            switch (face) {
                case Face::Right:
                case Face::Left:
                    sliceCount = CHUNK_SIZE;
                    uSize = CHUNK_SIZE;   // Z
                    vSize = CHUNK_HEIGHT; // Y
                    break;

                case Face::Top:
                case Face::Bottom:
                    sliceCount = CHUNK_HEIGHT;
                    uSize = CHUNK_SIZE;   // X
                    vSize = CHUNK_SIZE;   // Z
                    break;

                case Face::Front:
                case Face::Back:
                    sliceCount = CHUNK_SIZE;
                    uSize = CHUNK_SIZE;   // X
                    vSize = CHUNK_HEIGHT; // Y
                    break;
            }
            
            // The mask stores texture IDs.
            // -1 means empty / no visible face.
            std::vector<int> mask(uSize * vSize, -1);
            
            // Process each 2D slice
            for (int slice = 0; slice < sliceCount; ++slice) {
                // empty fill mask
                std::fill(mask.begin(), mask.end(), -1);
                
                // -------------------------------------------------
                // Step A: Build the visibility mask.
                // -------------------------------------------------
                for (int v = 0; v < vSize; ++v){
                    for (int u = 0; u < uSize; ++u) {
                        mask[v * uSize + u] = getVisibleTexture(face, slice, u, v); // fill up the mask with textureId
                    }
                }
                
                // -------------------------------------------------
                // Step B: Find and merge rectangles.
                // -------------------------------------------------
                for (int v = 0; v < vSize; ++v){
                    for (int u = 0; u < uSize; ++u) {
                        const int index = v * uSize + u;
                        const int tile = mask[index];
                        
                        if (tile < 0)
                            continue; // skip if texture is -1(empty)
                            
                        // -----------------------------------------
                        // Expand horizontally.
                        // Find consecutive faces with the same tile.
                        // -----------------------------------------
                        int width = 1;
                        while (u + width < uSize && mask[index + width] == tile){ // check if current mask offset == first tile's texture, continue until uSize
                            ++width;
                        }
                        
                        // -----------------------------------------
                        // Expand vertically.
                        // The entire next row must match.
                        // -----------------------------------------
                        int height = 1;
                        while (v + height < vSize){
                            bool canExpand = true;
                            
                            for (int du = 0; du < width; ++du){
                                if (mask[(v + height) * uSize + u + du] != tile){ // goes through the entire row & column of mask to check if there is any unmatch texture
                                    canExpand = false;
                                    break;
                                }
                            }
                            if (!canExpand)
                                break;

                            ++height;
                        }

                        // -----------------------------------------
                        // Mark merged cells as consumed.
                        // -----------------------------------------
                        for (int dv = 0; dv < height; ++dv){
                            for (int du = 0; du < width; ++du)
                            {
                                mask[(v + dv) * uSize + u + du] = -1;
                            }
                        }
                        
                        // -----------------------------------------
                        // Emit one large quad.
                        // -----------------------------------------
                        emitQuad(face, slice, u, v, width, height, tile);
                    }
                }
            }
        }
    } else
    { // Temporary hardcoded tiles (index = row*16 + col, see note below)
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
