#include "ChunkManager.hpp"
#include "Chunk.hpp"
#include "Constants.hpp"
#include "Mesh.hpp"
#include "Texture.hpp"
#include <memory>
#include <mutex>

ChunkManager::ChunkManager(int viewDistance)
    : viewDistance_(viewDistance)
    , blockTexturePath_("assets/test.png")
{
    // Spawn threads on initialize
    // Spawn N-1 worker threads (leave one core for main thread)
    const int numWorkers = std::max(1u, std::thread::hardware_concurrency() - 1); 
    for (int i = 0; i < numWorkers; ++i){
       workers_.emplace_back(&ChunkManager::WorkerLoop, this);
    }
}

ChunkManager::~ChunkManager()
{
    // Signal worker thread to stop, then join
    {
        std::lock_guard lock(workMutex_);
        stopping_ = true;
    }
    workCV_.notify_all(); // wake all worker thread
    for (auto& t: workers_) t.join(); // join individual thread
}

// ---------------------------------------------------------
// Main Thread

void ChunkManager::Update(const glm::vec3 &cameraPos, const Shader &shader, const Camera &camera)
{
    const int camChunkX = static_cast<int>(std::floor(cameraPos.x / CHUNK_SIZE));
    const int camChunkZ = static_cast<int>(std::floor(cameraPos.z / CHUNK_SIZE));

    // 1. Load Chunk that should exsist but dont
    for (int dx = -viewDistance_; dx <= viewDistance_; ++dx){
       for (int dz = -viewDistance_; dz <= viewDistance_; ++dz) {
           ChunkCoord coord{ camChunkX + dx, camChunkZ + dz  };
           

           /* Implement Spiral Outward Chunk loading order
           std::vector<ChunkCoord> loadQueue;

            for (int dx = -viewDistance_; dx <= viewDistance_; ++dx) {
                for (int dz = -viewDistance_; dz <= viewDistance_; ++dz) {
                    loadQueue.push_back({ camChunkX + dx, camChunkZ + dz });
                }
            }

            // Sort so chunks closest to the camera are at the FRONT of the vector
            std::sort(loadQueue.begin(), loadQueue.end(), [camChunkX, camChunkZ](const ChunkCoord& a, const ChunkCoord& b) {
                int distA = (a.x - camChunkX) * (a.x - camChunkX) + (a.z - camChunkZ) * (a.z - camChunkZ);
                int distB = (b.x - camChunkX) * (b.x - camChunkX) + (b.z - camChunkZ) * (b.z - camChunkZ);
                return distA < distB; 
            });

            // Now loop through your sorted queue to insert and EnqueueWork!
            for (const auto& coord : loadQueue) {
                if (chunks_.count(coord)) continue;
                // ... allocate chunk and EnqueueWork here ...
            }
           */
            Chunk* rawChunk = nullptr;
            { 
                std::lock_guard lock(chunksMutex_);
                if (chunks_.count(coord)) continue; // skip if the chunk already build

                // Allocate Chunk (no GL yet - happens on Upload if Ready)
                auto chunk =  std::make_unique<Chunk>(coord);
                rawChunk = chunk.get();
                chunks_[coord] = std::move(chunk);
            }

            // Push work onto thread pool
            EnqueueWork([this, rawChunk, coord](){
                    rawChunk->Generate();
                    rawChunk->BuildMesh(GatherNeighbors(coord));
                    {
                        std::lock_guard lock(uploadMutex_);
                        uploadQueue_.push(coord);
                    }
            });
            
       }
    }

    // 2. Unload Chunk outside view distance + a small buffer
    const int unloadDist = viewDistance_ + 2; // add 2 chunk buffer
    {
        std::lock_guard lock(chunksMutex_);
        for (auto it = chunks_.begin(); it != chunks_.end(); ){
            if (
                std::abs(it->first.x - camChunkX) >= unloadDist || 
                std::abs(it->first.z - camChunkZ) >= unloadDist 
            ){
                it = chunks_.erase(it);
            } else {
                ++it;
            }
        }
    }

    // 3. Re-enqueue Dirty chunks for remesh
    {
        std::lock_guard lock(chunksMutex_);
        for (auto& [coord, chunk] : chunks_) {
            if (chunk->State() == ChunkState::Dirty) {
                Chunk* raw = chunk.get();
                EnqueueWork([this, raw, coord]() {
                    raw->BuildMesh(GatherNeighbors(coord));
                    {
                        std::lock_guard ul(uploadMutex_);
                        uploadQueue_.push(coord);
                    }
                });
            }
        }
    }
    // 4. Drain upload queue on the main thread (GL calls here)
    int uploads = 0;
    while (uploads < maxUploadsPerFrame_) {
        ChunkCoord coord;
        {
            std::lock_guard lock(uploadMutex_);
            if (uploadQueue_.empty()) break; 
            coord = uploadQueue_.front();
            uploadQueue_.pop();
        }
        {
            std::lock_guard lock(chunksMutex_);
            auto it = chunks_.find(coord);
            if (it != chunks_.end())
                it->second->UploadIfReady();
        }
        ++uploads; 
    }

    // 5. Draw all uploaded chunk
    // Shader is already Activate()'d by the caller
    {
        std::lock_guard lock(chunksMutex_);
        for (auto& [coord, chunk] : chunks_){
            chunk->Render(shader, camera);
        }

    }
}

void ChunkManager::WorkerLoop()
{
    while (true){
        std::function<void()> fn;
        {
            std::unique_lock lock(workMutex_);
            workCV_.wait(lock, [&]{ return stopping_ || !workQueue_.empty(); }); // only wakes if stopping or something in workQueue
            if (stopping_ && workQueue_.empty()) return;
            // steal the work from queue
            fn = std::move(workQueue_.front()); 
            workQueue_.pop(); // remove the work from queue
        }
        fn(); // execute the work
    }
}

void ChunkManager::EnqueueWork(std::function<void()> fn){
    {
        std::lock_guard lock(workMutex_);
        workQueue_.push(std::move(fn)); // push into worker queue
    }
    workCV_.notify_one(); // and notify thread to begin work
}

const Chunk* ChunkManager::GetChunk(ChunkCoord coord) const {
    auto it = chunks_.find(coord);
    return it != chunks_.end() ? it->second.get() : nullptr; // get the chunk pointer, if not found nullptr
}

ChunkNeighbors ChunkManager::GatherNeighbors(ChunkCoord coord) const
{
    {
        std::lock_guard lock(chunksMutex_);
        return {
            GetChunk({coord.x + 1, coord.z}),
            GetChunk({coord.x - 1, coord.z}),
            GetChunk({coord.x, coord.z + 1}),
            GetChunk({coord.x, coord.z - 1}),
        };
    }
    
}
