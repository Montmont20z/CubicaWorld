#include "DebugOverlay.hpp"

#include "ChunkManager.hpp"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <algorithm>
#include <cstdint>
#include <vector>

bool DebugOverlay::Initialize(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
        ImGui::DestroyContext();
        return false;
    }
    if (!ImGui_ImplOpenGL3_Init("#version 430")) {
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        return false;
    }
    return true;
}

void DebugOverlay::Shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void DebugOverlay::BeginFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}


namespace {
struct BenchmarkCapture {
    bool valid = false;
    double frameAvg = 0.0;
    double frameP95 = 0.0;
    double updateAvg = 0.0;
    double updateP95 = 0.0;
    double gpuAvg = 0.0;
    double gpuP95 = 0.0;
    double meshAvg = 0.0;
    double meshP95 = 0.0;
    double meshJobsPerSecond = 0.0;
    std::uint64_t meshQuads = 0;
    std::uint64_t meshVertices = 0;
    std::uint64_t meshIndices = 0;
    bool meshGeometryValid = false;
    bool greedyMeshingEnabled = false;
    bool faceCullingEnabled = false;
    std::uint64_t drawnChunks = 0;
    std::uint64_t frustumRejected = 0;
    std::uint64_t horizonRejected = 0;
    int viewDistance = 0;
    bool frustumEnabled = false;
    bool horizonEnabled = false;
    bool singleThreaded = false;
};

BenchmarkCapture CaptureBenchmark(const PerformanceSnapshot& m) {
    BenchmarkCapture result;
    result.valid = true;
    result.updateAvg = m.chunkUpdateAvgMs;
    result.updateP95 = m.chunkUpdateP95Ms;
    result.gpuAvg = m.gpuChunkAvgMs;
    result.gpuP95 = m.gpuChunkP95Ms;
    result.meshAvg = m.meshBuildAvgMs;
    result.meshP95 = m.meshBuildP95Ms;
    result.meshJobsPerSecond = m.meshJobsPerSecond;
    result.meshGeometryValid = m.lastMesh.valid;
    result.meshQuads = m.lastMesh.quads;
    result.meshVertices = m.lastMesh.vertices;
    result.meshIndices = m.lastMesh.indices;
    result.greedyMeshingEnabled = m.lastMesh.greedyMeshingEnabled;
    result.faceCullingEnabled = m.lastMesh.faceCullingEnabled;
    result.drawnChunks = m.drawnChunks;
    result.frustumRejected = m.frustumRejected;
    result.horizonRejected = m.horizonRejectedChunks;
    result.viewDistance = m.viewDistance;
    result.frustumEnabled = m.frustumCullingEnabled;
    result.horizonEnabled = m.horizonCullingEnabled;
    result.singleThreaded = m.singleThreaded;

    std::vector<double> frames;
    frames.reserve(m.frameHistoryCount);
    for (std::size_t i = 0; i < m.frameHistoryCount; ++i) {
        frames.push_back(m.frameHistory[i]);
    }
    if (frames.empty()) frames.push_back(m.frameMs);
    double sum = 0.0;
    for (double value : frames) sum += value;
    result.frameAvg = sum / static_cast<double>(frames.size());
    std::sort(frames.begin(), frames.end());
    const std::size_t p95Index = static_cast<std::size_t>(0.95 * static_cast<double>(frames.size() - 1));
    result.frameP95 = frames[p95Index];
    return result;
}

void CompareValue(const char* label, double baseline, double current, const char* unit) {
    const double deltaPercent = baseline != 0.0
        ? ((current - baseline) / baseline) * 100.0 : 0.0;
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%.3f %s", baseline, unit);
    ImGui::TableSetColumnIndex(2);
    ImGui::Text("%.3f %s", current, unit);
    ImGui::TableSetColumnIndex(3);
    ImGui::Text("%+.1f%%", deltaPercent);
}

void CompareHigherIsBetter(const char* label, double baseline, double current, const char* unit) {
    const double deltaPercent = baseline != 0.0
        ? ((current - baseline) / baseline) * 100.0 : 0.0;
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%.2f %s", baseline, unit);
    ImGui::TableSetColumnIndex(2);
    ImGui::Text("%.2f %s", current, unit);
    ImGui::TableSetColumnIndex(3);
    ImGui::Text("%+.1f%%", deltaPercent);
}
}

static void MetricRow(const char* label, std::uint64_t value) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%llu", static_cast<unsigned long long>(value));
}

static void MetricRow(const char* label, double value, const char* unit) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%.3f %s", value, unit);
}

void DebugOverlay::Draw(ChunkManager& chunks, const PerformanceSnapshot& m) {
    ImGui::SetNextWindowSize(ImVec2(440.0f, 720.0f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("CubicaWorld | Performance", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::End();
        return;
    }

    ImGui::Text("F3: toggle overlay");
    ImGui::Separator();

    // Main frame counters.
    ImGui::Text("FPS: %.1f", m.fps);
    ImGui::SameLine(180.0f);
    ImGui::Text("Frame: %.2f ms", m.frameMs);
    ImGui::Text("ChunkManager::Update: %.3f ms (avg %.3f / p95 %.3f)",
        m.chunkUpdateMs, m.chunkUpdateAvgMs, m.chunkUpdateP95Ms);
    ImGui::Text("GPU chunk pass: %.3f ms (avg %.3f / p95 %.3f)",
        m.gpuChunkMs, m.gpuChunkAvgMs, m.gpuChunkP95Ms);
    if (m.frameHistoryCount > 1) {
        ImGui::PlotLines("Frame time (ms)", m.frameHistory.data(),
                         static_cast<int>(m.frameHistoryCount), 0, nullptr,
                         0.0f, 50.0f, ImVec2(-1.0f, 65.0f));
    }

    const BenchmarkCapture liveBenchmark = CaptureBenchmark(m);
    ImGui::Text("Frame rolling average: %.2f ms | p95: %.2f ms",
        liveBenchmark.frameAvg, liveBenchmark.frameP95);

    static BenchmarkCapture baseline;
    if (ImGui::Button("Capture baseline")) baseline = CaptureBenchmark(m);
    ImGui::SameLine();
    if (ImGui::Button("Clear baseline")) baseline = BenchmarkCapture{};
    if (baseline.valid) {
        const BenchmarkCapture current = CaptureBenchmark(m);
        ImGui::Text("Baseline: view %d | frustum %s | horizon %s | %s",
            baseline.viewDistance,
            baseline.frustumEnabled ? "ON" : "OFF",
            baseline.horizonEnabled ? "ON" : "OFF",
            baseline.singleThreaded ? "single-threaded" : "multithreaded");
        if (ImGui::BeginTable("benchmark_comparison", 4,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("Metric");
            ImGui::TableSetupColumn("Baseline");
            ImGui::TableSetupColumn("Current");
            ImGui::TableSetupColumn("Change");
            ImGui::TableHeadersRow();
            CompareValue("Frame average", baseline.frameAvg, current.frameAvg, "ms");
            CompareValue("Frame p95", baseline.frameP95, current.frameP95, "ms");
            CompareValue("Chunk update average", baseline.updateAvg, current.updateAvg, "ms");
            CompareValue("Chunk update p95", baseline.updateP95, current.updateP95, "ms");
            CompareValue("GPU chunk average", baseline.gpuAvg, current.gpuAvg, "ms");
            CompareValue("GPU chunk p95", baseline.gpuP95, current.gpuP95, "ms");
            CompareValue("Mesh build average", baseline.meshAvg, current.meshAvg, "ms");
            CompareValue("Mesh build p95", baseline.meshP95, current.meshP95, "ms");
            CompareHigherIsBetter("Mesh throughput", baseline.meshJobsPerSecond,
                                  current.meshJobsPerSecond, "chunks/s");
            ImGui::EndTable();
        }
        ImGui::Text("Current passed to Render(): %llu | Frustum rejected: %llu | Horizon rejected: %llu",
            static_cast<unsigned long long>(current.drawnChunks),
            static_cast<unsigned long long>(current.frustumRejected),
            static_cast<unsigned long long>(current.horizonRejected));
        if (current.meshGeometryValid && baseline.meshGeometryValid) {
            ImGui::Text("Last mesh output: baseline %llu quads / current %llu quads",
                static_cast<unsigned long long>(baseline.meshQuads),
                static_cast<unsigned long long>(current.meshQuads));
        }
        ImGui::TextWrapped("Negative timing change means the current configuration is faster. Wait for the rolling frame-time window to settle after changing a setting before capturing/comparing.");
    }

    if (ImGui::CollapsingHeader("World / chunks", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::BeginTable("world_metrics", 2, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Metric");
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 120.0f);
            MetricRow("View distance (chunks)", static_cast<std::uint64_t>(m.viewDistance));
            MetricRow("Target chunks (square)", m.targetChunks);
            MetricRow("Loaded chunks", m.loadedChunks);
            MetricRow("GPU-uploaded chunks", m.uploadedChunks);
            MetricRow("Not uploaded / not ready", m.notUploadedChunks);
            MetricRow("Chunks passed to Render()", m.drawnChunks);
            ImGui::EndTable();
        }
        if (m.gpuMemoryTrackingValid) {
            ImGui::Text("Resident mesh GPU memory: %.2f MiB",
                static_cast<double>(m.residentGpuBytes) / (1024.0 * 1024.0));
        } else {
            ImGui::Text("Resident chunk mesh buffers: no non-empty chunk mesh uploaded yet");
        }
        if (m.drawStatsValid) {
            ImGui::Text("Actual indexed draw calls: %llu | Submitted triangles: %llu",
                static_cast<unsigned long long>(m.drawCalls),
                static_cast<unsigned long long>(m.trianglesDrawn));
        } else {
            ImGui::Text("Actual indexed draw calls: 0 this frame");
        }
        const float denominator = m.loadedChunks > 0 ? static_cast<float>(m.loadedChunks) : 1.0f;
        ImGui::Text("Uploaded: %.1f%%", 100.0f * static_cast<float>(m.uploadedChunks) / denominator);
        ImGui::ProgressBar(static_cast<float>(m.uploadedChunks) / denominator, ImVec2(-1.0f, 0.0f));
    }

    if (ImGui::CollapsingHeader("Visibility / culling", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Pipeline: loaded -> uploaded -> frustum -> horizon -> Render()");
        if (ImGui::BeginTable("culling_metrics", 2, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Metric");
            ImGui::TableSetupColumn("Count", ImGuiTableColumnFlags_WidthFixed, 120.0f);
            MetricRow("Frustum tested", m.frustumTested);
            MetricRow("Frustum rejected", m.frustumRejected);
            MetricRow("Frustum passed", m.frustumPassed);
            MetricRow("Horizon candidate chunks", m.horizonTestedChunks);
            MetricRow("Horizon rejected chunks", m.horizonRejectedChunks);
            MetricRow("Horizon passed chunks", m.horizonPassedChunks);
            MetricRow("Horizon samples generated", m.horizonSamplesGenerated);
            MetricRow("Horizon samples evaluated", m.horizonSamplesEvaluated);
            ImGui::EndTable();
        }
        ImGui::Text("Frustum stage: %.3f ms", m.frustumStageMs);
        ImGui::Text("Horizon stage: %.3f ms", m.horizonStageMs);
        ImGui::Text("Interpretation: rejected chunks show eliminated work; frame/GPU time shows whether it helped.");
    }

    if (ImGui::CollapsingHeader("Meshing / thread pool", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Mode: %s", m.singleThreaded ? "Single-threaded" : "Multithreaded");
        if (ImGui::BeginTable("meshing_metrics", 2, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Metric");
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 120.0f);
            MetricRow("Worker threads", m.workerThreads);
            MetricRow("Active workers", m.activeWorkers);
            MetricRow("Pending mesh jobs", m.pendingMeshJobs);
            MetricRow("Pending GPU uploads", m.pendingUploads);
            MetricRow("Jobs queued (lifetime)", m.totalJobsQueued);
            MetricRow("Jobs completed (lifetime)", m.totalJobsCompleted);
            MetricRow("Mesh builds measured", m.totalMeshBuilds);
            MetricRow("Mesh jobs / second (1s window)", m.meshJobsPerSecond, "chunks/s");
            MetricRow("Average terrain generation", m.terrainGenerationAvgMs, "ms");
            MetricRow("Average mesh build", m.meshBuildAvgMs, "ms");
            MetricRow("p95 mesh build", m.meshBuildP95Ms, "ms");
            MetricRow("Last mesh build", m.meshBuildLastMs, "ms");
            MetricRow("Average queue wait", m.queueWaitAvgMs, "ms");
            MetricRow("Uploads this frame", m.uploadsThisFrame);
            MetricRow("Upload CPU time", m.uploadCpuMs, "ms");
            ImGui::EndTable();
        }
        ImGui::TextWrapped("Single vs. multithreaded comparison should use time to finish the same fixed mesh workload, not steady-state FPS alone.");
    }

    if (ImGui::CollapsingHeader("Mesh geometry / mesher")) {
        if (m.lastMesh.valid) {
            if (ImGui::BeginTable("geometry_metrics", 2, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Last completed chunk mesh");
                ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 150.0f);
                MetricRow("Final quads", m.lastMesh.quads);
                MetricRow("Triangles (indices / 3)", m.lastMesh.indices / 3);
                MetricRow("Vertices", m.lastMesh.vertices);
                MetricRow("Indices", m.lastMesh.indices);
                MetricRow("Vertex + index bytes", m.lastMesh.gpuBytes);
                MetricRow("Mesher execution time", m.lastMesh.mesherTotalMs, "ms");
                if (m.lastMesh.faceCountsValid) {
                    MetricRow("Faces considered", m.lastMesh.facesConsidered);
                    MetricRow("Faces emitted", m.lastMesh.facesEmitted);
                }
                if (m.lastMesh.faceStageTimingsValid) {
                    MetricRow("Face-culling stage", m.lastMesh.faceCullingMs, "ms");
                    MetricRow("Greedy-merge stage", m.lastMesh.greedyMeshingMs, "ms");
                }
                ImGui::EndTable();
            }
            if (m.lastMesh.mesherConfigValid) {
                const char* mesherName = m.lastMesh.greedyMeshingEnabled
                    ? "Greedy"
                    : (m.lastMesh.faceCullingEnabled ? "NaiveCulled" : "Naive");
                ImGui::Text("Configured mesher: %s | Naive hidden-face filter: %s",
                    mesherName, m.lastMesh.faceCullingEnabled ? "ON" : "OFF");
            }
            if (m.lastMesh.faceCountsValid && m.lastMesh.facesConsidered > 0) {
                const std::uint64_t eliminated = m.lastMesh.facesConsidered >= m.lastMesh.facesEmitted
                    ? m.lastMesh.facesConsidered - m.lastMesh.facesEmitted : 0;
                const double faceCullReduction = 100.0 * static_cast<double>(eliminated) /
                    static_cast<double>(m.lastMesh.facesConsidered);
                ImGui::Text("Voxel face-culling reduction: %.1f%%", faceCullReduction);
            } else {
                ImGui::TextWrapped("Faces considered/emitted are not available from final MeshData alone. Add counters inside NaiveMesher/GreedyMesher to measure these accurately.");
            }
            if (!m.lastMesh.faceStageTimingsValid) {
                ImGui::TextWrapped("This hook measures total mesher execution. Isolated face-culling and greedy-merge timings require timers inside the corresponding mesher implementations.");
            }
        } else {
            ImGui::TextWrapped("Waiting for the first completed chunk mesh.");
        }
        if (m.gpuMemoryTrackingValid) {
            ImGui::Text("Resident chunk-mesh GPU buffers: %.2f MiB",
                static_cast<double>(m.residentGpuBytes) / (1024.0 * 1024.0));
        } else {
            ImGui::Text("Resident chunk-mesh GPU buffers: waiting for first upload");
        }
    }

    if (ImGui::CollapsingHeader("Runtime optimization controls", ImGuiTreeNodeFlags_DefaultOpen)) {
        bool frustum = chunks.IsFrustumCullingEnabled();
        if (ImGui::Checkbox("Frustum culling", &frustum)) chunks.SetFrustumCullingEnabled(frustum);

        bool horizon = chunks.IsHorizonCullingEnabled();
        if (ImGui::Checkbox("Horizon / occlusion culling", &horizon)) chunks.SetHorizonCullingEnabled(horizon);

        int viewDistance = chunks.GetViewDistance();
        if (ImGui::SliderInt("Chunk view distance", &viewDistance, 2, 32)) {
            chunks.SetViewDistance(viewDistance);
        }

        ImGui::Separator();
        ImGui::Text("OpenGL back-face culling: %s", m.glBackFaceCullingEnabled ? "ON" : "OFF");
        ImGui::Text("Threading mode: %s (set at construction)", m.singleThreaded ? "single" : "multi");
        if (m.lastMesh.mesherConfigValid) {
            ImGui::Text("Mesher mode and NaiveMesher hidden-face filter shown under Mesh geometry.");
            ImGui::Text("Meshing mode is currently selected by RenderSettings.hpp at build time.");
        } else {
            ImGui::Text("Meshing configuration will appear after the first completed chunk mesh.");
        }
    }

    if (ImGui::CollapsingHeader("Timing notes")) {
        ImGui::Text("Render CPU time: %.3f ms", m.renderCpuMs);
        ImGui::TextWrapped("GPU chunk pass is an asynchronous GL_TIME_ELAPSED query around ChunkManager::Update. It includes GL uploads and chunk draws; CPU-only terrain generation/meshing does not consume GPU time.");
        ImGui::TextWrapped("V-Sync should be disabled for uncapped performance comparisons. Keep it enabled for normal gameplay if desired.");
    }

    ImGui::End();
}

void DebugOverlay::Render() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}