#pragma once
#include <atomic>
#include <memory>
#include "Camera.hpp"
#include "ChunkMesh.hpp"
#include "BlockType.hpp"
#include "MeshData.hpp"
#include "Constants.hpp"
#include "Shader.hpp"



struct ChunkCoord {
    int x { 0 };
    int z { 0 };
    bool operator==(const ChunkCoord& o) const { return x == o.x && z == o.z; } // overwrite == operation , allow simply code from "if (cc1.x == cc2.x && cc1.z == cc2.z) TO if (cc1 == cc2) {}"
};

struct ChunkCoordHash{
    std::size_t operator()(const ChunkCoord& c)const noexcept {
        return std::hash<int>()(c.x) ^ (std::hash<int>()(c.z) << 16);
        // Alternative method, less likely to hash collision on large number, but will keep implementation simple now
        /*std::size_t h1 = std::hash<int>()(c.x);
        std::size_t h2 = std::hash<int>()(c.z);
        // A mathematically proven way to combine hashes with low collision rates
        return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));*/
    }
};

enum class ChunkState : uint8_t {
    Empty,     // allocated blocks not genereted
    Generated, // blocks_ filled, mesh not built
    MeshReady, // MeshData built on worker threadm waiting for GL upload
    Uploaded,  // GPU mesh is current, safe to render
    Dirty,     // block changed - needs remesh (reruns BuildMesh -> MeshReady)
};

class Chunk;
// Neighbors passed to BuildMesh so border faces can be culled correctly.
// Null = neighbor not loaded; treat as solid (never emit that face)
struct ChunkNeighbors {
    std::shared_ptr<const Chunk> px;// +X
    std::shared_ptr<const Chunk> nx; // -X
    std::shared_ptr<const Chunk> pz; // +Z
    std::shared_ptr<const Chunk> nz; // -Z
};



class Chunk {
public:
    // Chunk(std::vector<Vertex> &vertices, std::vector<GLuint> &indices, std::vector<std::unique_ptr<Texture>> &&textures, GLenum usage);
    // Take ownership of texture. coord identifies world position
    // Chunk(ChunkCoord coord, std::vector<std::unique_ptr<Texture>> &&textures);
    explicit Chunk(ChunkCoord coord);
    
    // ----------------- Called by worker threads -------------------------
    
    // Fill blocks_ with solid data (flat fill for now; swap in Perlin later).
    void Generate();
    
    // Read blocks_ + neighbor blocks, write into pendingMesh_.
    void BuildMesh(const ChunkNeighbors& neighbors = {});
    
    // ----------------- Called by main thread only -----------------------
    
    // If state == MeshReady, upload pendingMesh_ to GPU and set state = Uploaded.
    // Clears pendingMesh_ after upload to free CPU memory.
    void UploadIfReady();
    // Draw. Shader must already be Activate()'d by caller.
    // model matrix is derived from coord — no parameters needed.
    void Render(const Shader& shader, const Camera& camera) const;
    
    // --- Accessor ---
    ChunkCoord Coord() const { return coord_; };
    ChunkState State() const { return state_.load(std::memory_order_acquire); }
    BlockType GetBlock(int x, int y, int z) const; 
    void SetBlock(int x, int y, int z, BlockType type); 
    void MarkDirty(){ state_.store(ChunkState::Dirty, std::memory_order_release); };


private:
    ChunkCoord coord_;
    std::atomic<ChunkState> state_{ ChunkState::Empty };

    // BlockType blocks_[CHUNK_SIZE][CHUNK_HEIGHT][CHUNK_SIZE] = { BlockType::Dirt };
    // heap-allocated so Chunk is cheap to store in unordered_map and future moves
    // Flat array: index = x*CHUNK_HEIGHT*CHUNK_SIZE + y*CHUNK_SIZE + z
    std::unique_ptr<BlockType[]> blocks_; // size = CHUNK_SIZE * CHUNK_HEIGH * CHUNK_SIZE // use this instead of std::vector<BlockType> because it is fixed size, has lower memory overhead // not using BlockType[] blocks_; because StackOverFlow 
    BlockType& BlockAt(int x, int y, int z){ return blocks_[x + y * CHUNK_SIZE * CHUNK_SIZE + z * CHUNK_SIZE]; } // this = Chunk*       → non-const overload → returns BlockType& (writable)
    const BlockType& BlockAt(int x, int y, int z) const { return blocks_[x + y * CHUNK_SIZE * CHUNK_SIZE + z * CHUNK_SIZE]; } // this = const Chunk* → const overload     → returns const BlockType& (read-only)

    ChunkMesh chunkMesh_;                                        // GPU object, main thread only
    std::vector<std::unique_ptr<Texture>> textures_;
    MeshData pendingMesh_;
};
