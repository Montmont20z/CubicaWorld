#pragma once
#include <stdint.h>

enum class BlockType : uint8_t {
    Air = 0,
    Grass, 
    Dirt,
    Stone,
};