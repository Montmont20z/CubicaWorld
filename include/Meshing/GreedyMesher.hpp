// GreedyMesher.hpp
#pragma once

#include <cstdint>
#include <vector>

#include "BlockSampler.hpp"
#include "FaceData.hpp"
#include "MeshData.hpp"

namespace meshing {

// Greedy mesher for one chunk.
//
// Create one on the STACK per BuildMesh call. It owns a scratch mask buffer,
// so sharing an instance between worker threads would be a data race.
class GreedyMesher {
public:
    explicit GreedyMesher(const BlockSampler& sampler);

    void Build(MeshData& out);

private:
    // Texture tile of the face visible at mask cell (u, v) of this slice, or -1 for none.
    int  VisibleTile(Face f, int slice, int u, int v) const;

    // Step A: fill mask_ for one slice.
    void BuildMask(Face f, int slice, const FaceDims& d);

    // Step B: walk mask_, merge into rectangles, emit quads.
    void ExtractRects(Face f, int slice, const FaceDims& d);

    int  MeasureWidth (int u, int v, int tile, const FaceDims& d) const;
    int  MeasureHeight(int u, int v, int width, int tile, const FaceDims& d) const;
    void ConsumeRect  (int u, int v, int width, int height, const FaceDims& d);

    void EmitQuad(Face f, int slice, int u, int v, int width, int height, uint8_t tile);

    const BlockSampler& s_;
    MeshData*           out_ = nullptr;
    std::vector<int>    mask_;   // -1 = no face, otherwise tile index. Reused for every slice/face.
};

} // namespace meshing