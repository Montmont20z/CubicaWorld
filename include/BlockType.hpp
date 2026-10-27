#pragma once
#include <stdint.h>

enum class BlockType : uint8_t {
    Air = 0,
    Grass, 
    Dirt,
    Stone,
    Log,
    Leaf,
    Sand,
    Water,
    Cactus,
    Flower,
    ShortGrass,
    DeadBush,
};