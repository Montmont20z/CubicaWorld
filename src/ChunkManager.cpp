#include "ChunkManager.hpp"
#include "Chunk.hpp"
#include "Constants.hpp"
#include "RenderSettings.hpp"
#include "Logger.hpp"
#include "OcclusionCuller.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

ChunkManager::ChunkManager(int viewDistance, bool isSingleThreaded)
    : viewDistance_(viewDistance)
    , isSingleThreaded_(isSingleThreaded)
{
    // Spawn threads on initialize
    // Spawn N-1 worker threads (leave one core for main thread)
    if (!isSingleThreaded_){ // skip thread initialize if single thread
        const int numWorkers = std::max(1u, std::thread::hardware_concurrency() - 1); 
        for (int i = 0; i < numWorkers; ++i){
           workers_.emplace_back(&ChunkManager::WorkerLoop, this);
        }
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
    chunks_.clear();  // drop all refs; deleter just queues pointers now, no GL calls yet
    DrainPendingDeletes(); // actually delete them here, on the main thread
}

// ---------------------------------------------------------
// Main Thread

void ChunkManager::Update(const glm::vec3 &cameraPos, const Shader &shader, const Camera &camera)
{
    if (isSingleThreaded_) UpdateSingleThreaded(cameraPos, shader, camera);
    else                 UpdateMultiThreaded(cameraPos, shader, camera);
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

std::shared_ptr<const Chunk> ChunkManager::GetChunk(ChunkCoord coord) const {
    auto it = chunks_.find(coord);
    return it != chunks_.end() ? it->second : nullptr; // get the chunk pointer, if not found nullptr
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


std::shared_ptr<Chunk> ChunkManager::MakeChunk(ChunkCoord coord)
{
    return std::shared_ptr<Chunk>(new Chunk(coord), [this](Chunk* c){
        std::lock_guard lock(deleteMutex_);
        pendingDeletes_.push_back(c);
    });
}

void ChunkManager::DrainPendingDeletes()
{
    std::vector<Chunk*> toDelete;
    {
        std::lock_guard lock(deleteMutex_);
        toDelete.swap(pendingDeletes_);
    }
    for (Chunk* c: toDelete) delete c;  // ~Chunk() -> ~ChunkMesh() GL deletes: main thread only, safe here
}

void ChunkManager::UpdateSingleThreaded(const glm::vec3& cameraPos,const Shader& shader, const Camera& camera){
    
    const int camChunkX = static_cast<int>(std::floor(cameraPos.x / CHUNK_SIZE));
    const int camChunkZ = static_cast<int>(std::floor(cameraPos.z / CHUNK_SIZE));

    // 1. Load Chunk that should exist but dont
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
            std::shared_ptr<Chunk> rawChunk = nullptr;
            { 
                std::lock_guard lock(chunksMutex_);
                if (chunks_.count(coord)) continue; // skip if the chunk already build

                // Allocate Chunk (no GL yet - happens on Upload if Ready)
                rawChunk = MakeChunk(coord);
                chunks_[coord] = rawChunk;
            }

            // single thread draw chunk
            rawChunk->Generate(terrainGenerator_);
            rawChunk->BuildMesh(GatherNeighbors(coord));
            rawChunk->UploadIfReady(); // upload Mesh to GPU
       }
    }

    // Single thread remesh & render
    // 2. Unload Chunk outside view distance + a small buffer
    // iterate through all chunks
    const int unloadDist = viewDistance_ + 2;
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
    DrainPendingDeletes(); // destructor of Chunk 

    // 3. Remesh Dirty chunks
    for (auto& [coord, chunk ]: chunks_){
       if (chunk->GetChunkState() == ChunkState::Dirty){
           chunk->BuildMesh(GatherNeighbors(coord));
           chunk->UploadIfReady(); // push dirty mesh to GPU
       } 
    }
    
    // 4. Filter all chunk that does not pass the optimization filter & Sort for rendering order
    Frustum frustum = camera.GetFrustum(); // build once per frame, outside the loop

    std::vector<RenderCandidate> candidates;
    candidates.reserve(chunks_.size());

    // push back chunk into candidates if it pass all optimization filter
    for (auto& [coord, chunk]: chunks_){
        if (chunk->GetChunkState() != ChunkState::Uploaded)
            continue;

        if (IS_FRUSTUM_CULLING && !frustum.IsVisible(chunk->GetBoundingBox()) ) // skip glDraw call if chunk not within view frustum 
            continue;
       
        const AABB bounds = chunk->GetBoundingBox(); 

        // find the closest point between camera and chunk footprint
        const float closestX = std::clamp(
            cameraPos.x,
            bounds.min.x,
            bounds.max.x
        );
        const float closestZ = std::clamp(
            cameraPos.z,
            bounds.min.z,
            bounds.max.z
        );
        const float dx = closestX - cameraPos.x;
        const float dz = closestZ - cameraPos.z;

        const float nearestDistanceSq = dx * dx + dz * dz;

        candidates.push_back({
            coord,
            chunk,
            nearestDistanceSq,
            !IS_HORIZON_CULLING 
        }); 
    }
    
    if (IS_HORIZON_CULLING){
        // Sort chunks from nearest to farthest.
        std::sort(
            candidates.begin(),
            candidates.end(),
            [](const RenderCandidate& a, const RenderCandidate& b){
                return a.nearestDistanceSq < b.nearestDistanceSq; // sort by distanceSq ascending
            }
        );

        // Generate Terrain sample 
        std::vector<HorizonSample> samples;

        for (std::size_t i = 0; i < candidates.size(); ++i){
            const RenderCandidate& candidate = candidates[i];
            for (int x = 0; x < CHUNK_SIZE; ++x){
                for (int z = 0; z < CHUNK_SIZE; ++z){
                    
                    const float sampleWorldX = candidate.coord.x * CHUNK_SIZE + x ;
                    const float sampleWorldZ = candidate.coord.z * CHUNK_SIZE + z ;

                    // Center of the block, used for distance calculations.
                    const float worldX = sampleWorldX + 0.5f;
                    const float worldZ = sampleWorldZ + 0.5f;

                    const float worldY = static_cast<float>(candidate.chunk->GetSurfaceHeight(sampleWorldX, sampleWorldZ));
                    
                    const float dx = worldX - cameraPos.x;
                    const float dz = worldZ - cameraPos.z;
                    
                    const float distanceSq = dx * dx + dz * dz;
                    
                    // Avoid unstable elevation calculations near the camera.
                    if (distanceSq < 1.0f){
                        candidates[i].visible = true;
                        continue;
                    }
                    
                    const float distance = std::sqrt(distanceSq);

                    // Elevation relative to the camera's horizontal plane.
                    const float elevation = std::atan2(worldY - cameraPos.y, distance);

                    const int bin = GetAzimuthBin(dx, dz);

                    samples.push_back({
                        distanceSq,
                        elevation,
                        bin,
                        i
                    });
                }
            }
        }
        
        // Sort terrain samples by distance
        std::sort(
            samples.begin(),
            samples.end(),
            [](const HorizonSample& a,
               const HorizonSample& b)
            {
                return a.distanceSq < b.distanceSq;
            }
        );

        
        // Horizontal culling 
        Horizon horizon;
        horizon.fill(-PI * 0.5f);
        
        for (const HorizonSample& sample: samples){
            const float previousHorizon = horizon[sample.bin];

            // Is this terrain sample potentially visible?
            if (sample.elevation >= previousHorizon - HORIZON_MARGIN){
                candidates[sample.candidateIndex].visible = true;
            }
            
            // Update the skyline if this sample extends it.
            if (sample.elevation > previousHorizon){
                horizon[sample.bin] = sample.elevation;
            }
        }
    }
    
    // 5. Draw all chunk
    drawnChunks_ = 0;
    for (RenderCandidate candidate: candidates){
        if (!candidate.visible) // skip render if not visible
            continue;

        candidate.chunk->Render(shader, camera);
        ++drawnChunks_;
    }
    totalChunks_ = static_cast<int>(chunks_.size());
    if (prevChunkCount_ != drawnChunks_){
        LOG_DEBUG("Total Chunk Drawn: " + std::to_string(drawnChunks_) + " \nTotal Chunk: " + std::to_string(totalChunks_));
        prevChunkCount_ = drawnChunks_;
    }
}

void ChunkManager::UpdateMultiThreaded(const glm::vec3& cameraPos,const Shader& shader, const Camera& camera){
    
    const int camChunkX = static_cast<int>(std::floor(cameraPos.x / CHUNK_SIZE));
    const int camChunkZ = static_cast<int>(std::floor(cameraPos.z / CHUNK_SIZE));
    
    // int worldX = camChunkX * CHUNK_SIZE + cameraPos.x;

    // TODO: replace with in game overlay
    // std::cout << "Current Chunk X: " << camChunkX << "  Current Chunk Z: " << camChunkZ << std::endl;
    // std::cout << "Current X: " << cameraPos.x << "  Current Y: " << cameraPos.y << "  Current Z: " << cameraPos.z << std::endl;

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
            std::shared_ptr<Chunk> rawChunk = nullptr;
            { 
                std::lock_guard lock(chunksMutex_);
                if (chunks_.count(coord)) continue; // skip if the chunk already build

                // Allocate Chunk (no GL yet - happens on Upload if Ready)
                // auto chunk =  std::make_unique<Chunk>(coord);
                rawChunk = MakeChunk(coord);
                chunks_[coord] = rawChunk;
            }

            // Push work onto thread pool
            EnqueueWork([this, rawChunk, coord](){
                    rawChunk->Generate(terrainGenerator_);
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
    DrainPendingDeletes(); // safe destructor of Chunk 

    // 3. Re-enqueue Dirty chunks for remesh
    {
        std::lock_guard lock(chunksMutex_);
        for (auto& [coord, chunk] : chunks_) {
            if (chunk->GetChunkState() == ChunkState::Dirty) {
                std::shared_ptr<Chunk> sharedChunk = chunk;
                EnqueueWork([this, sharedChunk, coord]() {
                    sharedChunk->BuildMesh(GatherNeighbors(coord));
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

    // 5. Filter all chunk that does not pass the optimization filter & Sort for rendering order
    Frustum frustum = camera.GetFrustum(); // build once per frame, outside the loop

    std::vector<RenderCandidate> candidates;
    candidates.reserve(chunks_.size());

    // push back chunk into candidates if it pass all optimization filter
    for (auto& [coord, chunk]: chunks_){
        if (chunk->GetChunkState() != ChunkState::Uploaded)
            continue;

        if (IS_FRUSTUM_CULLING && !frustum.IsVisible(chunk->GetBoundingBox()) ) // skip glDraw call if chunk not within view frustum 
            continue;
       
        const AABB bounds = chunk->GetBoundingBox(); 

        // find the closest point between camera and chunk footprint
        const float closestX = std::clamp(
            cameraPos.x,
            bounds.min.x,
            bounds.max.x
        );
        const float closestZ = std::clamp(
            cameraPos.z,
            bounds.min.z,
            bounds.max.z
        );
        const float dx = closestX - cameraPos.x;
        const float dz = closestZ - cameraPos.z;

        const float nearestDistanceSq = dx * dx + dz * dz;

        candidates.push_back({
            coord,
            chunk,
            nearestDistanceSq,
            !IS_HORIZON_CULLING 
        }); 
    }
    
    if (IS_HORIZON_CULLING){
        // Sort chunks from nearest to farthest.
        std::sort(
            candidates.begin(),
            candidates.end(),
            [](const RenderCandidate& a, const RenderCandidate& b){
                return a.nearestDistanceSq < b.nearestDistanceSq; // sort by distanceSq ascending
            }
        );

        // Generate Terrain sample 
        std::vector<HorizonSample> samples;

        for (std::size_t i = 0; i < candidates.size(); ++i){
            const RenderCandidate& candidate = candidates[i];
            for (int x = 0; x < CHUNK_SIZE; ++x){
                for (int z = 0; z < CHUNK_SIZE; ++z){
                    
                    const float sampleWorldX = candidate.coord.x * CHUNK_SIZE + x ;
                    const float sampleWorldZ = candidate.coord.z * CHUNK_SIZE + z ;

                    // Center of the block, used for distance calculations.
                    const float worldX = sampleWorldX + 0.5f;
                    const float worldZ = sampleWorldZ + 0.5f;

                    const float worldY = static_cast<float>(candidate.chunk->GetSurfaceHeight(sampleWorldX, sampleWorldZ));
                    
                    const float dx = worldX - cameraPos.x;
                    const float dz = worldZ - cameraPos.z;
                    
                    const float distanceSq = dx * dx + dz * dz;
                    
                    // Avoid unstable elevation calculations near the camera.
                    if (distanceSq < 1.0f){
                        candidates[i].visible = true;
                        continue;
                    }
                    
                    const float distance = std::sqrt(distanceSq);

                    // Elevation relative to the camera's horizontal plane.
                    const float elevation = std::atan2(worldY - cameraPos.y, distance);

                    const int bin = GetAzimuthBin(dx, dz);

                    samples.push_back({
                        distanceSq,
                        elevation,
                        bin,
                        i
                    });
                }
            }
        }
        
        // Sort terrain samples by distance
        std::sort(
            samples.begin(),
            samples.end(),
            [](const HorizonSample& a,
               const HorizonSample& b)
            {
                return a.distanceSq < b.distanceSq;
            }
        );

        
        // Horizontal culling 
        Horizon horizon;
        horizon.fill(-PI * 0.5f);
        
        for (const HorizonSample& sample: samples){
            const float previousHorizon = horizon[sample.bin];

            // Is this terrain sample potentially visible?
            if (sample.elevation >= previousHorizon - HORIZON_MARGIN){
                candidates[sample.candidateIndex].visible = true;
            }
            
            // Update the skyline if this sample extends it.
            if (sample.elevation > previousHorizon){
                horizon[sample.bin] = sample.elevation;
            }
        }
    }


    // 6. Draw all uploaded chunk
    // Shader is already Activate()'d by the caller
    {
        std::lock_guard lock(chunksMutex_);
        drawnChunks_ = 0;
        for (RenderCandidate candidate: candidates){
            if (!candidate.visible) // skip render if not visible
                continue;

            candidate.chunk->Render(shader, camera);
            ++drawnChunks_;
        }
        totalChunks_ = static_cast<int>(chunks_.size());
        if (prevChunkCount_ != drawnChunks_){
            LOG_DEBUG("Total Chunk Drawn: " + std::to_string(drawnChunks_) + " \nTotal Chunk: " + std::to_string(totalChunks_));
            prevChunkCount_ = drawnChunks_;
        }
    }
}
