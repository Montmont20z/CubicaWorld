#include "Meshing/GreedyMesher.hpp"

#include <algorithm>
#include <cassert>
#include <glad/glad.h>
#include <glm/glm.hpp>

#include "BlockType.hpp"
#include "Constants.hpp"

namespace meshing {
namespace {
    constexpr int AO_FULL = 2; // placeholder until the AO pass exists
}

GreedyMesher::GreedyMesher(const BlockSampler& sampler)
    : s_(sampler)
{
}

void GreedyMesher::Build(MeshData& out)
{
    out_ = &out;

    // Big enough for the largest mask (X/Z faces: CHUNK_SIZE * CHUNK_HEIGHT).
    // Allocated once, then reused for every slice of every face.
    mask_.assign(static_cast<size_t>(CHUNK_SIZE) * CHUNK_HEIGHT, -1);

    for (Face face : ALL_FACES) {
        const FaceDims d = DimsFor(face);
        for (int slice = 0; slice < d.sliceCount; ++slice) {
            BuildMask(face, slice, d);
            ExtractRects(face, slice, d);
        }
    }

    out_ = nullptr;
}

// ---------------------------------------------------------------------------
// Step A: visibility mask
// ---------------------------------------------------------------------------

int GreedyMesher::VisibleTile(Face f, int slice, int u, int v) const
{
    const glm::ivec3 p = SliceToBlock(f, slice, u, v);

    const BlockType block = s_.Get(p.x, p.y, p.z);
    if (block == BlockType::Air) return -1;

    // Only emit faces exposed to air.
    const glm::ivec3 n = p + NormalOf(f);
    if (!s_.IsAir(n.x, n.y, n.z)) return -1;

    return static_cast<int>(TileFor(block, f));
}

void GreedyMesher::BuildMask(Face f, int slice, const FaceDims& d)
{
    // Every cell is overwritten, so no separate clear pass is needed.
    for (int v = 0; v < d.vSize; ++v)
        for (int u = 0; u < d.uSize; ++u)
            mask_[v * d.uSize + u] = VisibleTile(f, slice, u, v);
}

// ---------------------------------------------------------------------------
// Step B: rectangle merging
// ---------------------------------------------------------------------------

// Grow right along u while the tile matches.
int GreedyMesher::MeasureWidth(int u, int v, int tile, const FaceDims& d) const
{
    const int row = v * d.uSize;
    int width = 1;
    while (u + width < d.uSize && mask_[row + u + width] == tile)
        ++width;
    return width;
}

// Grow up along v while the ENTIRE next row segment matches.
int GreedyMesher::MeasureHeight(int u, int v, int width, int tile, const FaceDims& d) const
{
    int height = 1;
    while (v + height < d.vSize) {
        const int row = (v + height) * d.uSize + u;
        for (int du = 0; du < width; ++du) {
            if (mask_[row + du] != tile)
                return height;
        }
        ++height;
    }
    return height;
}

// Mark merged cells as used so they aren't emitted twice.
void GreedyMesher::ConsumeRect(int u, int v, int width, int height, const FaceDims& d)
{
    for (int dv = 0; dv < height; ++dv) {
        const int row = (v + dv) * d.uSize + u;
        std::fill_n(mask_.begin() + row, width, -1);
    }
}

void GreedyMesher::ExtractRects(Face f, int slice, const FaceDims& d)
{
    for (int v = 0; v < d.vSize; ++v) {
        for (int u = 0; u < d.uSize; ++u) {
            const int tile = mask_[v * d.uSize + u];
            if (tile < 0) continue;

            const int width  = MeasureWidth(u, v, tile, d);
            const int height = MeasureHeight(u, v, width, tile, d);

            ConsumeRect(u, v, width, height, d);
            EmitQuad(f, slice, u, v, width, height, static_cast<uint8_t>(tile));
        }
    }
}

// ---------------------------------------------------------------------------
// Quad emission
// ---------------------------------------------------------------------------

void GreedyMesher::EmitQuad(Face face, int slice, int u, int v, int width, int height, uint8_t tile)
{
    glm::vec3 positions[4];
    glm::vec2 uvs[4];
    glm::vec3 normal(0.0f);

    const float S = static_cast<float>(slice);
    const float U = static_cast<float>(u);
    const float V = static_cast<float>(v);
    const float W = static_cast<float>(width);
    const float H = static_cast<float>(height);

    switch (face) {
        case Face::Right:  // +X   (u = Z, v = Y)
            normal = {1,0,0};
            positions[0] = { S + 0.5f, V    , U - 0.5f     };
            positions[1] = { S + 0.5f, V + H, U - 0.5f     };
            positions[2] = { S + 0.5f, V + H, U + W - 0.5f };
            positions[3] = { S + 0.5f, V    , U + W - 0.5f };
            uvs[0] = {0,0}; uvs[1] = {0,H}; uvs[2] = {W,H}; uvs[3] = {W,0};
            break;

        case Face::Left:   // -X   (u = Z, v = Y)
            normal = {-1,0,0};
            positions[0] = { S - 0.5f, V    , U + W - 0.5f };
            positions[1] = { S - 0.5f, V + H, U + W - 0.5f };
            positions[2] = { S - 0.5f, V + H, U - 0.5f     };
            positions[3] = { S - 0.5f, V    , U - 0.5f     };
            uvs[0] = {0,0}; uvs[1] = {0,H}; uvs[2] = {W,H}; uvs[3] = {W,0};
            break;

        case Face::Top:    // +Y   (u = X, v = Z)
            normal = {0,1,0};
            positions[0] = { U - 0.5f    , S + 1.0f, V + H - 0.5f };
            positions[1] = { U + W - 0.5f, S + 1.0f, V + H - 0.5f };
            positions[2] = { U + W - 0.5f, S + 1.0f, V - 0.5f     };
            positions[3] = { U - 0.5f    , S + 1.0f, V - 0.5f     };
            uvs[0] = {0,0}; uvs[1] = {W,0}; uvs[2] = {W,H}; uvs[3] = {0,H};
            break;

        case Face::Bottom: // -Y   (u = X, v = Z)
            normal = {0,-1,0};
            positions[0] = { U - 0.5f    , S, V - 0.5f     };
            positions[1] = { U + W - 0.5f, S, V - 0.5f     };
            positions[2] = { U + W - 0.5f, S, V + H - 0.5f };
            positions[3] = { U - 0.5f    , S, V + H - 0.5f };
            uvs[0] = {0,0}; uvs[1] = {W,0}; uvs[2] = {W,H}; uvs[3] = {0,H};
            break;

        case Face::Front:  // +Z   (u = X, v = Y)
            normal = {0,0,1};
            positions[0] = { U - 0.5f    , V    , S + 0.5f };
            positions[1] = { U + W - 0.5f, V    , S + 0.5f };
            positions[2] = { U + W - 0.5f, V + H, S + 0.5f };
            positions[3] = { U - 0.5f    , V + H, S + 0.5f };
            uvs[0] = {0,0}; uvs[1] = {W,0}; uvs[2] = {W,H}; uvs[3] = {0,H};
            break;

        case Face::Back:   // -Z   (u = X, v = Y)
            normal = {0,0,-1};
            positions[0] = { U + W - 0.5f, V + H, S - 0.5f };
            positions[1] = { U - 0.5f    , V + H, S - 0.5f };
            positions[2] = { U - 0.5f    , V    , S - 0.5f };
            positions[3] = { U + W - 0.5f, V    , S - 0.5f };
            uvs[0] = {0,H}; uvs[1] = {W,H}; uvs[2] = {W,0}; uvs[3] = {0,0};
            break;

        default:
            assert(false && "Invalid Face");
            return;
    }

    const auto base = static_cast<GLuint>(out_->vertices.size());

    for (int i = 0; i < 4; ++i)
        out_->vertices.push_back({ positions[i], normal, uvs[i], tile, AO_FULL });

    // Two CCW triangles: 0-1-2, 2-3-0
    out_->indices.push_back(base + 0); out_->indices.push_back(base + 1); out_->indices.push_back(base + 2);
    out_->indices.push_back(base + 2); out_->indices.push_back(base + 3); out_->indices.push_back(base + 0);
}

} // namespace meshing