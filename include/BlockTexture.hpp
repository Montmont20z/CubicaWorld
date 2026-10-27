#pragma once
#include <cstddef>
#include <cstdint>
#include "BlockType.hpp"
enum class Face : uint8_t { Right, Left, Top, Bottom, Front, Back };

struct BlockTiles { uint8_t top, bottom, side; };

inline constexpr BlockTiles kBlockTiles[] = {
    /* Air */ // Air block is skipped
    /* Grass */ { 0,  2,  1 },
    /* Dirt  */ { 2,  2,  2 },
    /* Stone */ { 3,  3,  3 },
    /* Log   */ { 5, 5, 4 },
    /* Leaf  */ { 6,6, 6 },
    /* Sand  */ { 7, 7, 7 },
    /* Flower */ { 8, 8, 8 },
    /* ShortGrass */ { 9,9, 9 },
    /* DeadBush */ { 10,10,10},
};

static_assert(sizeof(kBlockTiles) / sizeof(kBlockTiles[0]) == static_cast<size_t>(BlockType::DeadBush),
                "kBlockTiles must consist each BlockType");


constexpr uint8_t TileFor(BlockType b, Face f){
    const BlockTiles& t = kBlockTiles[static_cast<size_t>(b) - 1];
    return f == Face::Top ? t.top : f == Face::Bottom ? t.bottom : t.side;
}