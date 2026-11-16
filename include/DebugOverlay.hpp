#pragma once

#include "PerformanceMetrics.hpp"

class ChunkManager;
struct GLFWwindow;

class DebugOverlay {
public:
    static bool Initialize(GLFWwindow* window);
    static void Shutdown();
    static void BeginFrame();
    static void Draw(ChunkManager& chunks, const PerformanceSnapshot& metrics);
    static void Render();
};
