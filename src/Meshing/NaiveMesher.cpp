#include "Meshing/NaiveMesher.hpp"

#include <cstdint>
#include <glad/glad.h>
#include <glm/glm.hpp>

#include "BlockType.hpp"
#include "Constants.hpp"
#include "FaceData.hpp"

namespace meshing {
namespace {

// Face templates in block-local space; translated per block in AppendFace.
// texLayer=0, ao=3 (full bright) are placeholders overwritten/ignored later.
const ChunkVertex RIGHT_FACE[4] = {
    {{ 0.5f, 0.0f, -0.5f}, { 1,0,0}, {0,0}, 0, 3},
    {{ 0.5f, 1.0f, -0.5f}, { 1,0,0}, {0,1}, 0, 3},
    {{ 0.5f, 1.0f,  0.5f}, { 1,0,0}, {1,1}, 0, 3},
    {{ 0.5f, 0.0f,  0.5f}, { 1,0,0}, {1,0}, 0, 3},
};
const ChunkVertex LEFT_FACE[4] = {
    {{-0.5f, 0.0f,  0.5f}, {-1,0,0}, {0,0}, 0, 3},
    {{-0.5f, 1.0f,  0.5f}, {-1,0,0}, {0,1}, 0, 3},
    {{-0.5f, 1.0f, -0.5f}, {-1,0,0}, {1,1}, 0, 3},
    {{-0.5f, 0.0f, -0.5f}, {-1,0,0}, {1,0}, 0, 3},
};
const ChunkVertex TOP_FACE[4] = {
    {{-0.5f, 1.0f,  0.5f}, {0,1,0}, {0,0}, 0, 3},
    {{ 0.5f, 1.0f,  0.5f}, {0,1,0}, {1,0}, 0, 3},
    {{ 0.5f, 1.0f, -0.5f}, {0,1,0}, {1,1}, 0, 3},
    {{-0.5f, 1.0f, -0.5f}, {0,1,0}, {0,1}, 0, 3},
};
const ChunkVertex BOTTOM_FACE[4] = {
    {{-0.5f, 0.0f, -0.5f}, {0,-1,0}, {0,0}, 0, 3},
    {{ 0.5f, 0.0f, -0.5f}, {0,-1,0}, {1,0}, 0, 3},
    {{ 0.5f, 0.0f,  0.5f}, {0,-1,0}, {1,1}, 0, 3},
    {{-0.5f, 0.0f,  0.5f}, {0,-1,0}, {0,1}, 0, 3},
};
const ChunkVertex FRONT_FACE[4] = {
    {{-0.5f, 0.0f,  0.5f}, {0,0,1}, {0,0}, 0, 3},
    {{ 0.5f, 0.0f,  0.5f}, {0,0,1}, {1,0}, 0, 3},
    {{ 0.5f, 1.0f,  0.5f}, {0,0,1}, {1,1}, 0, 3},
    {{-0.5f, 1.0f,  0.5f}, {0,0,1}, {0,1}, 0, 3},
};
const ChunkVertex BACK_FACE[4] = {
    {{ 0.5f, 0.0f, -0.5f}, {0,0,-1}, {0,0}, 0, 3},
    {{-0.5f, 0.0f, -0.5f}, {0,0,-1}, {1,0}, 0, 3},
    {{-0.5f, 1.0f, -0.5f}, {0,0,-1}, {1,1}, 0, 3},
    {{ 0.5f, 1.0f, -0.5f}, {0,0,-1}, {0,1}, 0, 3},
};

struct FaceDef {
    const ChunkVertex* verts;
    Face               face;
};

// One table instead of 12 near-identical lines.
// The neighbour offset comes from NormalOf(face), so there's no second copy to desync.
const FaceDef FACES[6] = {
    { RIGHT_FACE,  Face::Right  },
    { LEFT_FACE,   Face::Left   },
    { TOP_FACE,    Face::Top    },
    { BOTTOM_FACE, Face::Bottom },
    { FRONT_FACE,  Face::Front  },
    { BACK_FACE,   Face::Back   },
};

void AppendFace(const ChunkVertex face[4], uint8_t tile, int bx, int by, int bz, MeshData& out)
{
    const auto base = static_cast<GLuint>(out.vertices.size());
    const glm::vec3 offset(static_cast<float>(bx),
                           static_cast<float>(by),
                           static_cast<float>(bz));

    for (int i = 0; i < 4; ++i) {
        ChunkVertex v = face[i];
        v.position += offset;
        v.texLayer = tile;
        out.vertices.push_back(v);
    }

    // Two CCW triangles: 0-1-2, 2-3-0
    out.indices.push_back(base + 0); out.indices.push_back(base + 1); out.indices.push_back(base + 2);
    out.indices.push_back(base + 2); out.indices.push_back(base + 3); out.indices.push_back(base + 0);
}

} // namespace

void MeshNaive(const BlockSampler& s, bool cull, MeshData& out)
{
    for (int x = 0; x < CHUNK_SIZE; ++x) {
        for (int y = 0; y < CHUNK_HEIGHT; ++y) {
            for (int z = 0; z < CHUNK_SIZE; ++z) {
                const BlockType b = s.Get(x, y, z);
                if (b == BlockType::Air) continue;

                for (const FaceDef& f : FACES) {
                    // Culling is just one extra condition; naive == culled with the check removed.
                    if (cull) {
                        const glm::ivec3 n = NormalOf(f.face);
                        if (!s.IsAir(x + n.x, y + n.y, z + n.z)) continue;
                    }
                    AppendFace(f.verts, static_cast<uint8_t>(TileFor(b, f.face)), x, y, z, out);
                }
            }
        }
    }
}

} // namespace meshing