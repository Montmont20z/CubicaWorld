
#pragma once
/**
 TODO: 
 cadidate terrain type: Tundra, Taiga  // cold
        Forest, Swamp  // temperate
        Savanna,  Jungle  // hot
 add structure: tree, cactus, tallgrass, flower, big rocks, ore viens, 
 add snow cap lines: y> SNOW_LINES
 Caves?? villages? floating island? water?? lava??  
*/

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