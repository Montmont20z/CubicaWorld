
#pragma once

#include "BlockType.hpp"
#include "Constants.hpp"
#include <array>

enum class Biome {
    Desert,
    Grassland,
    Rockland,
};

enum class Landform {
    Plains,
    Hills,
    Mountains,
};

struct TerrainColumn {
    std::array<BlockType, CHUNK_HEIGHT> blocks;
    
    Biome biome;
    Landform landform;
    int surfaceY;
};

class TerrainGenerator {
    public:
        TerrainColumn GenerateColumn(int worldX, int worldZ) const;
};