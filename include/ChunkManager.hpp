/*
Producer-Consumer / Thread Pool design pattern
Do the heavy math on background threads, and do the rendering/GPU uploads exclusively on the main thread.
*/
#pragma once
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>

#include "Chunk.hpp"
#include "Shader.hpp"
#include "Camera.hpp"
#include "RenderSettings.hpp"

class ChunkManager {
public:
    explicit ChunkManager(int viewDistance = 8, bool isSingleThreaded = false);
    ~ChunkManager(); // joins worker threads

    // Call once per frame on the main thread
    // 1. Determines which chunk should exist based on camera position 
    // 2. Enqueues Generate+Build Mesh for new chunks on the thread pool
    // 3. Drains upload queue (max maxUploadsPerFrame_ GL uploads)
    // 4. Draw all uploaded chunks.
    void Update(const glm::vec3& cameraPos, const Shader& shader, const Camera& camera);
    void WorkerLoop();
    void EnqueueWork(std::function<void()> fn);

    std::shared_ptr<const Chunk> GetChunk(ChunkCoord coord) const;
    ChunkNeighbors GatherNeighbors(ChunkCoord coord) const;

    // Debug overlay controls. Call these from the main thread only.
    int GetViewDistance() const noexcept { return viewDistance_; }
    void SetViewDistance(int distance) noexcept;
    bool IsSingleThreaded() const noexcept { return isSingleThreaded_; }
    bool IsFrustumCullingEnabled() const noexcept { return frustumCullingEnabled_; }
    bool IsHorizonCullingEnabled() const noexcept { return horizonCullingEnabled_; }
    void SetFrustumCullingEnabled(bool enabled) noexcept { frustumCullingEnabled_ = enabled; }
    void SetHorizonCullingEnabled(bool enabled) noexcept { horizonCullingEnabled_ = enabled; }

private:
    struct WorkItem {
        std::function<void()> fn;
        std::chrono::steady_clock::time_point enqueuedAt;
    };

    // Chunk's destructor deletes GL (VAO/VBO/EBO), which is only legal on the main thread.
    // A worker thread can end up dropping the last shared_ptr ref to a Chunk
    // (eg a neighbor ChunkNeighbors temporary going out of scope inside a BuildMesh job
    // right after the main thread evicts that chunk), so the raw ~Chunk() call must never run there.
    // This custom deleter defers actual deletion by queuing the raw pointer; DrainPendingDeletes()
    // (called from Update(), main thread only) does the real 'delete'.
    std::shared_ptr<Chunk> MakeChunk(ChunkCoord coord);
    void DrainPendingDeletes();
    void UpdateSingleThreaded(const glm::vec3& cameraPos, const Shader& shader, const Camera& camera);
    void UpdateMultiThreaded(const glm::vec3& cameraPos, const Shader& shader, const Camera& camera);
    std::size_t PendingWorkCount();
    std::size_t PendingUploadCount();
    bool CanMesh(ChunkCoord c) const;
    bool TryGatherReadyNeighbors(ChunkCoord c, ChunkNeighbors& out) const;

    std::vector<Chunk*> pendingDeletes_;
    std::mutex deleteMutex_;

    std::unordered_map<ChunkCoord, std::shared_ptr<Chunk>, ChunkCoordHash> chunks_;
    // std::vector<std::unique_ptr<Chunk>> chunks_; // option 2, but only support fixed size world, and no negative index
    mutable std::mutex chunksMutex_;

    // -- Upload queue: worker pushes coord, main thread drains --
    std::queue<ChunkCoord> uploadQueue_;
    std::mutex uploadMutex_;

    // -- Thread pool --
    std::vector<std::thread> workers_;
    std::queue<WorkItem> workQueue_;
    std::condition_variable workCV_;
    std::mutex workMutex_;
    bool stopping_{ false };

    TerrainGenerator terrainGenerator_;
    bool isSingleThreaded_;
    int viewDistance_;
    bool frustumCullingEnabled_ = IS_FRUSTUM_CULLING;
    bool horizonCullingEnabled_ = IS_HORIZON_CULLING;

    int maxUploadsPerFrame_ = 4;
    int drawnChunks_ = 0;
};