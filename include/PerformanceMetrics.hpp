#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <vector>

// Metrics shared between the main thread and the chunk worker threads.
// This is a debug/benchmark facility; do not use it as gameplay state.
struct MeshGeometryMetrics {
    bool valid = false;
    std::uint64_t facesConsidered = 0;
    std::uint64_t facesEmitted = 0;
    std::uint64_t quads = 0;
    std::uint64_t vertices = 0;
    std::uint64_t indices = 0;
    std::uint64_t gpuBytes = 0;
    // These counters require instrumentation inside the mesher loops. They stay
    // unavailable until that instrumentation is added; never infer them from
    // final greedy geometry.
    bool faceCountsValid = false;
    bool faceStageTimingsValid = false;
    bool mesherConfigValid = false;
    bool faceCullingEnabled = false;
    bool greedyMeshingEnabled = false;
    double mesherTotalMs = 0.0;
    double faceCullingMs = 0.0;
    double greedyMeshingMs = 0.0;
};

struct PerformanceSnapshot {
    // Frame and CPU/GPU timing (milliseconds).
    double frameMs = 0.0;
    double fps = 0.0;
    double chunkUpdateMs = 0.0;
    double chunkUpdateAvgMs = 0.0;
    double chunkUpdateP95Ms = 0.0;
    double gpuChunkMs = 0.0;
    double gpuChunkAvgMs = 0.0;
    double gpuChunkP95Ms = 0.0;
    double frustumStageMs = 0.0;
    double horizonStageMs = 0.0;
    double uploadCpuMs = 0.0;
    double renderCpuMs = 0.0;
    double terrainGenerationAvgMs = 0.0;
    double meshBuildAvgMs = 0.0;
    double meshBuildP95Ms = 0.0;
    double queueWaitAvgMs = 0.0;
    double meshBuildLastMs = 0.0;
    double meshJobsPerSecond = 0.0;

    // World and visibility.
    int viewDistance = 0;
    std::uint64_t targetChunks = 0;
    std::uint64_t loadedChunks = 0;
    std::uint64_t uploadedChunks = 0;
    std::uint64_t notUploadedChunks = 0;
    std::uint64_t frustumTested = 0;
    std::uint64_t frustumRejected = 0;
    std::uint64_t frustumPassed = 0;
    std::uint64_t horizonTestedChunks = 0;
    std::uint64_t horizonRejectedChunks = 0;
    std::uint64_t horizonPassedChunks = 0;
    std::uint64_t horizonSamplesGenerated = 0;
    std::uint64_t horizonSamplesEvaluated = 0;
    std::uint64_t drawnChunks = 0;
    std::uint64_t drawCalls = 0;
    std::uint64_t trianglesDrawn = 0;
    bool drawStatsValid = false;
    std::uint64_t residentGpuBytes = 0;
    bool gpuMemoryTrackingValid = false;
    std::uint64_t viewUploadedChunks = 0;

    // Thread-pool and upload pipeline.
    bool singleThreaded = false;
    std::uint32_t workerThreads = 0;
    std::uint32_t activeWorkers = 0;
    std::uint64_t pendingMeshJobs = 0;
    std::uint64_t pendingUploads = 0;
    std::uint64_t totalJobsQueued = 0;
    std::uint64_t totalJobsCompleted = 0;
    std::uint64_t totalMeshBuilds = 0;
    std::uint64_t uploadsThisFrame = 0;

    // Current runtime/GL state.
    bool frustumCullingEnabled = false;
    bool horizonCullingEnabled = false;
    bool glBackFaceCullingEnabled = false;

    // Geometry values are populated by a hook inside Chunk::BuildMesh / ChunkMesh.
    MeshGeometryMetrics lastMesh;

    static constexpr std::size_t HistorySize = 600;
    std::array<float, HistorySize> frameHistory{}; // oldest -> newest
    std::size_t frameHistoryCount = 0;
};

class PerformanceMetrics {
public:
    static PerformanceMetrics& Instance();

    void BeginFrame();
    void RecordFrameTime(double milliseconds);
    void SetChunkUpdateTime(double milliseconds);
    void SetGpuChunkTime(double milliseconds);
    void SetFrustumStageTime(double milliseconds);
    void SetHorizonStageTime(double milliseconds);
    void SetViewUploadedChunks(std::uint64_t uploaded);
    void SetVisibilityCounts(
        std::uint64_t loaded,
        std::uint64_t uploaded,
        std::uint64_t frustumTested,
        std::uint64_t frustumRejected,
        std::uint64_t horizonTested,
        std::uint64_t horizonRejected,
        std::uint64_t horizonSamplesGenerated,
        std::uint64_t horizonSamplesEvaluated,
        std::uint64_t drawn);
    void SetWorldAndSettings(int viewDistance, bool singleThreaded,
                             std::uint32_t workerThreads,
                             bool frustumEnabled, bool horizonEnabled,
                             bool glBackFaceCullingEnabled);
    void SetQueueDepths(std::uint64_t pendingJobs, std::uint64_t pendingUploads);
    void AddUpload(double milliseconds);
    void SetRenderCpuTime(double milliseconds);
    void RecordDrawStatistics(std::uint64_t drawCalls, std::uint64_t triangles);
    void AdjustResidentGpuBytes(std::int64_t deltaBytes);

    // Called by EnqueueWork/WorkerLoop. These methods are thread-safe.
    void RecordJobQueued();
    void RecordWorkerStarted(double queueWaitMilliseconds);
    void RecordWorkerFinished();
    // Count synchronous chunk work in single-threaded mode without reporting
    // the main thread as a background worker.
    void RecordSynchronousJobCompleted();

    // Called immediately around Generate() and BuildMesh().
    void RecordTerrainGeneration(double milliseconds);
    void RecordMeshBuild(double milliseconds);

    // Optional hook from the mesher, once the output counts are known.
    void RecordMeshGeometry(const MeshGeometryMetrics& geometry);

    PerformanceSnapshot GetSnapshot() const;

private:
    PerformanceMetrics() = default;
    static constexpr std::size_t SampleWindow = 256;

    static void PushSample(std::vector<double>& samples, double value);
    static double Average(const std::vector<double>& samples);
    static double Percentile95(std::vector<double> samples);
    void UpdateThroughputLocked();

    mutable std::mutex mutex_;
    PerformanceSnapshot current_;
    std::vector<double> updateSamples_;
    std::vector<double> gpuSamples_;
    std::vector<double> meshBuildSamples_;
    std::vector<double> terrainGenerationSamples_;
    std::vector<double> queueWaitSamples_;
    std::array<float, PerformanceSnapshot::HistorySize> frameHistory_{};
    std::size_t frameHistoryHead_ = 0;
    std::size_t frameHistoryCount_ = 0;
    std::chrono::steady_clock::time_point throughputWindowStart_ =
        std::chrono::steady_clock::now();
    std::uint64_t throughputWindowCompletedStart_ = 0;
};