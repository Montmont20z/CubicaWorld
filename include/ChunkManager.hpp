/*
Producer-Consumer / Thread Pool design pattern
Do the heavy math on background threads, and do the rendering/GPU uploads exclusively on the main thread.
*/
#pragma once
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <thread>
#include <condition_variable>
#include <queue>
#include <functional>

#include "Chunk.hpp"
#include "Shader.hpp"
#include "Camera.hpp"

class ChunkManager {
public:
    explicit ChunkManager(int viewDistance = 8);
    ~ChunkManager(); // joins worker threads

    // Call once per frame on the main thread
    // 1. Determines which chunk should exist based on camera position 
    // 2. Enqueues Generate+Build Mesh for new chunks on the thread pool
    // 3. Drains upload queue (max maxUploadsPerFrame_ GL uploads)
    // 4. Draw all uploaded chunks.
    void Update(const glm::vec3& cameraPos, const Shader& shader, const Camera& camera);
                     
    void WorkerLoop();


    void EnqueueWork(std::function<void()> fn);
    const Chunk* GetChunk(ChunkCoord coord) const;
    ChunkNeighbors GatherNeighbors(ChunkCoord coord) const;

private:
    
    // Chunks
    //use std::unique_ptr<Chunk> to allow Chunk load & unload 
    std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>, ChunkCoordHash> chunks_; // for infinite world, but worst cache locality // can be optimise to use hybird hashmap for global, vector for local chunk
    // std::vector<std::unique_ptr<Chunk>> chunks_; // option 2, but only support fixed size world, and no negative index
    mutable std::mutex chunksMutex_;
    
    /// TODO: replace with shared TextureManager + GL_TEXTURE_2D_ARRAY before greedy meshing.
    std::string blockTexturePath_;
    
    // -- Upload queue: worker pushes coord, main thread drains --
    std::queue<ChunkCoord> uploadQueue_;
    std::mutex uploadMutex_; 

    
    // -- Thread pool --
    std::vector<std::thread>        workers_;
    std::queue<std::function<void()>> workQueue_;
    std::condition_variable         workCV_;
    std::mutex                      workMutex_;
    bool                            stopping_{ false };
    

    int viewDistance_;
    int maxUploadsPerFrame_ = 4;
};