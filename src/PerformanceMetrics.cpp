#include "PerformanceMetrics.hpp"

#include <algorithm>
#include <cmath>

PerformanceMetrics& PerformanceMetrics::Instance() {
    static PerformanceMetrics instance;
    return instance;
}

void PerformanceMetrics::PushSample(std::vector<double>& samples, double value) {
    if (!std::isfinite(value) || value < 0.0) return;
    samples.push_back(value);
    if (samples.size() > SampleWindow) {
        samples.erase(samples.begin(), samples.begin() + (samples.size() - SampleWindow));
    }
}

double PerformanceMetrics::Average(const std::vector<double>& samples) {
    if (samples.empty()) return 0.0;
    double sum = 0.0;
    for (double value : samples) sum += value;
    return sum / static_cast<double>(samples.size());
}

double PerformanceMetrics::Percentile95(std::vector<double> samples) {
    if (samples.empty()) return 0.0;
    std::sort(samples.begin(), samples.end());
    const std::size_t index = static_cast<std::size_t>(std::ceil(0.95 * samples.size())) - 1;
    return samples[std::min(index, samples.size() - 1)];
}

void PerformanceMetrics::BeginFrame() {
    std::lock_guard<std::mutex> lock(mutex_);
    UpdateThroughputLocked();
    current_.chunkUpdateMs = 0.0;
    current_.frustumStageMs = 0.0;
    current_.horizonStageMs = 0.0;
    current_.uploadCpuMs = 0.0;
    current_.renderCpuMs = 0.0;
    current_.uploadsThisFrame = 0;
    current_.loadedChunks = 0;
    current_.uploadedChunks = 0;
    current_.notUploadedChunks = 0;
    current_.frustumTested = 0;
    current_.frustumRejected = 0;
    current_.frustumPassed = 0;
    current_.horizonTestedChunks = 0;
    current_.horizonRejectedChunks = 0;
    current_.horizonPassedChunks = 0;
    current_.horizonSamplesGenerated = 0;
    current_.horizonSamplesEvaluated = 0;
    current_.drawnChunks = 0;
    current_.drawCalls = 0;
    current_.trianglesDrawn = 0;
    current_.drawStatsValid = false;
}

void PerformanceMetrics::RecordFrameTime(double milliseconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    current_.frameMs = milliseconds;
    current_.fps = milliseconds > 0.0 ? 1000.0 / milliseconds : 0.0;
    frameHistory_[frameHistoryHead_] = static_cast<float>(milliseconds);
    frameHistoryHead_ = (frameHistoryHead_ + 1) % frameHistory_.size();
    frameHistoryCount_ = std::min(frameHistoryCount_ + 1, frameHistory_.size());
}

void PerformanceMetrics::SetChunkUpdateTime(double milliseconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    current_.chunkUpdateMs = milliseconds;
    PushSample(updateSamples_, milliseconds);
    current_.chunkUpdateAvgMs = Average(updateSamples_);
    current_.chunkUpdateP95Ms = Percentile95(updateSamples_);
}

void PerformanceMetrics::SetGpuChunkTime(double milliseconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    current_.gpuChunkMs = milliseconds;
    PushSample(gpuSamples_, milliseconds);
    current_.gpuChunkAvgMs = Average(gpuSamples_);
    current_.gpuChunkP95Ms = Percentile95(gpuSamples_);
}

void PerformanceMetrics::SetFrustumStageTime(double milliseconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    current_.frustumStageMs = milliseconds;
}

void PerformanceMetrics::SetHorizonStageTime(double milliseconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    current_.horizonStageMs = milliseconds;
}

void PerformanceMetrics::SetVisibilityCounts(
    std::uint64_t loaded,
    std::uint64_t uploaded,
    std::uint64_t frustumTested,
    std::uint64_t frustumRejected,
    std::uint64_t horizonTested,
    std::uint64_t horizonRejected,
    std::uint64_t horizonSamplesGenerated,
    std::uint64_t horizonSamplesEvaluated,
    std::uint64_t drawn) {
    std::lock_guard<std::mutex> lock(mutex_);
    current_.loadedChunks = loaded;
    current_.uploadedChunks = uploaded;
    current_.notUploadedChunks = loaded >= uploaded ? loaded - uploaded : 0;
    current_.frustumTested = frustumTested;
    current_.frustumRejected = frustumRejected;
    current_.frustumPassed = frustumTested >= frustumRejected
        ? frustumTested - frustumRejected : 0;
    current_.horizonTestedChunks = horizonTested;
    current_.horizonRejectedChunks = horizonRejected;
    current_.horizonPassedChunks = horizonTested >= horizonRejected
        ? horizonTested - horizonRejected : 0;
    current_.horizonSamplesGenerated = horizonSamplesGenerated;
    current_.horizonSamplesEvaluated = horizonSamplesEvaluated;
    current_.drawnChunks = drawn;
}

void PerformanceMetrics::SetWorldAndSettings(
    int viewDistance, bool singleThreaded, std::uint32_t workerThreads,
    bool frustumEnabled, bool horizonEnabled, bool glBackFaceCullingEnabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    current_.viewDistance = viewDistance;
    current_.targetChunks = static_cast<std::uint64_t>(2 * viewDistance + 1) *
                            static_cast<std::uint64_t>(2 * viewDistance + 1);
    current_.singleThreaded = singleThreaded;
    current_.workerThreads = workerThreads;
    current_.frustumCullingEnabled = frustumEnabled;
    current_.horizonCullingEnabled = horizonEnabled;
    current_.glBackFaceCullingEnabled = glBackFaceCullingEnabled;
}

void PerformanceMetrics::SetQueueDepths(std::uint64_t pendingJobs, std::uint64_t pendingUploads) {
    std::lock_guard<std::mutex> lock(mutex_);
    current_.pendingMeshJobs = pendingJobs;
    current_.pendingUploads = pendingUploads;
}

void PerformanceMetrics::AddUpload(double milliseconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    current_.uploadCpuMs += milliseconds;
    ++current_.uploadsThisFrame;
}

void PerformanceMetrics::SetRenderCpuTime(double milliseconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    current_.renderCpuMs = milliseconds;
}

void PerformanceMetrics::RecordDrawStatistics(std::uint64_t drawCalls, std::uint64_t triangles) {
    std::lock_guard<std::mutex> lock(mutex_);
    current_.drawCalls += drawCalls;
    current_.trianglesDrawn += triangles;
    current_.drawStatsValid = true;
}

void PerformanceMetrics::AdjustResidentGpuBytes(std::int64_t deltaBytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    current_.gpuMemoryTrackingValid = true;
    if (deltaBytes >= 0) {
        current_.residentGpuBytes += static_cast<std::uint64_t>(deltaBytes);
    } else {
        const std::uint64_t decrease = static_cast<std::uint64_t>(-deltaBytes);
        current_.residentGpuBytes = decrease > current_.residentGpuBytes
            ? 0 : current_.residentGpuBytes - decrease;
    }
}

void PerformanceMetrics::RecordJobQueued() {
    std::lock_guard<std::mutex> lock(mutex_);
    ++current_.totalJobsQueued;
}

void PerformanceMetrics::RecordWorkerStarted(double queueWaitMilliseconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    ++current_.activeWorkers;
    PushSample(queueWaitSamples_, queueWaitMilliseconds);
    current_.queueWaitAvgMs = Average(queueWaitSamples_);
}

void PerformanceMetrics::RecordWorkerFinished() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (current_.activeWorkers > 0) --current_.activeWorkers;
    ++current_.totalJobsCompleted;
    UpdateThroughputLocked();
}

void PerformanceMetrics::RecordSynchronousJobCompleted() {
    std::lock_guard<std::mutex> lock(mutex_);
    ++current_.totalJobsQueued;
    ++current_.totalJobsCompleted;
    UpdateThroughputLocked();
}

void PerformanceMetrics::UpdateThroughputLocked() {
    const auto now = std::chrono::steady_clock::now();
    const double elapsedSeconds = std::chrono::duration<double>(
        now - throughputWindowStart_).count();
    if (elapsedSeconds < 1.0) return;

    const std::uint64_t completed = current_.totalJobsCompleted;
    const std::uint64_t delta = completed >= throughputWindowCompletedStart_
        ? completed - throughputWindowCompletedStart_ : 0;
    current_.meshJobsPerSecond = static_cast<double>(delta) / elapsedSeconds;
    throughputWindowCompletedStart_ = completed;
    throughputWindowStart_ = now;
}

void PerformanceMetrics::RecordTerrainGeneration(double milliseconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    PushSample(terrainGenerationSamples_, milliseconds);
    current_.terrainGenerationAvgMs = Average(terrainGenerationSamples_);
}

void PerformanceMetrics::RecordMeshBuild(double milliseconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    PushSample(meshBuildSamples_, milliseconds);
    current_.meshBuildLastMs = milliseconds;
    current_.meshBuildAvgMs = Average(meshBuildSamples_);
    current_.meshBuildP95Ms = Percentile95(meshBuildSamples_);
    ++current_.totalMeshBuilds;
}

void PerformanceMetrics::RecordMeshGeometry(const MeshGeometryMetrics& geometry) {
    std::lock_guard<std::mutex> lock(mutex_);
    current_.lastMesh = geometry;
    current_.lastMesh.valid = true;
}

PerformanceSnapshot PerformanceMetrics::GetSnapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    PerformanceSnapshot result = current_;
    result.frameHistoryCount = frameHistoryCount_;
    for (std::size_t i = 0; i < frameHistoryCount_; ++i) {
        const std::size_t oldest = (frameHistoryHead_ + frameHistory_.size() - frameHistoryCount_) % frameHistory_.size();
        result.frameHistory[i] = frameHistory_[(oldest + i) % frameHistory_.size()];
    }
    return result;
}