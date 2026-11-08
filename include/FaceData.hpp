#pragma once
#include <array>
#include <glm/glm.hpp>

#include "BlockTexture.hpp"  // Face enum + TileFor() -- adjust if Face lives elsewhere
#include "Constants.hpp"     // CHUNK_SIZE, CHUNK_HEIGHT

// Single source of truth for how a (face, slice, u, v) mask coordinate maps
// to a block coordinate. Both the visibility test and the quad emitter use
// this, so the "u/v swapped on Top/Bottom" class of bug can't come back.

namespace meshing {

inline constexpr std::array<Face, 6> ALL_FACES = {
    Face::Right, Face::Left, Face::Top, Face::Bottom, Face::Front, Face::Back
};

struct FaceDims {
    int sliceCount; // how many slices along the fixed axis
    int uSize;      // mask width
    int vSize;      // mask height
};

inline FaceDims DimsFor(Face f) {
    switch (f) {
        case Face::Right:
        case Face::Left:   return { CHUNK_SIZE,   CHUNK_SIZE, CHUNK_HEIGHT }; // slice=X, u=Z, v=Y
        case Face::Top:
        case Face::Bottom: return { CHUNK_HEIGHT, CHUNK_SIZE, CHUNK_SIZE   }; // slice=Y, u=X, v=Z
        case Face::Front:
        case Face::Back:   return { CHUNK_SIZE,   CHUNK_SIZE, CHUNK_HEIGHT }; // slice=Z, u=X, v=Y
    }
    return { 0, 0, 0 };
}

// Mask coordinate -> block (x, y, z)
inline glm::ivec3 SliceToBlock(Face f, int slice, int u, int v) {
    switch (f) {
        case Face::Right:
        case Face::Left:   return { slice, v, u };
        case Face::Top:
        case Face::Bottom: return { u, slice, v };
        case Face::Front:
        case Face::Back:   return { u, v, slice };
    }
    return { 0, 0, 0 };
}

// Offset to the neighbouring block this face looks at
inline glm::ivec3 NormalOf(Face f) {
    switch (f) {
        case Face::Right:  return {  1,  0,  0 };
        case Face::Left:   return { -1,  0,  0 };
        case Face::Top:    return {  0,  1,  0 };
        case Face::Bottom: return {  0, -1,  0 };
        case Face::Front:  return {  0,  0,  1 };
        case Face::Back:   return {  0,  0, -1 };
    }
    return { 0, 0, 0 };
}

} // namespace meshing