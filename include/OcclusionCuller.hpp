/* 
horizontal culling 
As you traverse terrain from near to far, maintain the visible skyline. 
If a distant chunk is entirely below that skyline, it cannot be seen, so skip rendering it.

1. Angular horizon
θ = tan^-1 (y-yCamera / distance)
*/
#pragma once

#include "Chunk.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <memory>
namespace {
    constexpr int HORIZONTAL_BIN_COUNT = 4096;
    constexpr float PI = 3.14159265358979323846f;
    constexpr float TWO_PI = 2.0f * PI;
    
    // A visibility safety margin.
    // Larger values mean more conservative culling.
    constexpr float HORIZON_MARGIN = 2.0f * PI / 180.0f; // additional 2.0f degree

    using Horizon = std::array<float, HORIZONTAL_BIN_COUNT>;
    
    struct RenderCandidate {
        ChunkCoord coord;
        std::shared_ptr<Chunk> chunk;

        float nearestDistanceSq = 0.0f;
        bool visible = false;
    };
    
    struct HorizonSample {
        float distanceSq;
        float elevation;

        int bin;
        std::size_t candidateIndex;
    };

    // group samples by angular bin
    using SamplesByBin = std::array<std::vector<HorizonSample>, HORIZONTAL_BIN_COUNT>;
    
    int GetAzimuthBin(float dx, float dz){
        const float angle = std::atan2(dz, dx); // atan2 to handle all four quadrants, negative to positive angles
        
        const float normalize = (angle + PI) / TWO_PI; // turn -pi to +pi   Into   0 to 1 

        const int bin = static_cast<int>(normalize * HORIZONTAL_BIN_COUNT); // turn 0 to 1   Into actual angle from -180 degree to +180 degree

        return std::clamp(
            bin, 
            0,
            HORIZONTAL_BIN_COUNT - 1
        );
    }
} // namespace