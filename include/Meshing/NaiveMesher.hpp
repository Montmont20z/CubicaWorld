#pragma once

#include "BlockSampler.hpp"
#include "MeshData.hpp"

namespace meshing {

// One quad per visible block face.
//   cull == false -> emits all 6 faces of every solid block (baseline for benchmarks)
//   cull == true  -> only emits faces whose neighbour is air (face culling)
void MeshNaive(const BlockSampler& sampler, bool cull, MeshData& out);

} // namespace meshing