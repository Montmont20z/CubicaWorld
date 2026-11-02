#include "TerrainGenerator.hpp"
#include <algorithm>
#include <cassert>
#include <glm/vec2.hpp>
#include <glm/gtc/noise.hpp>

namespace {

struct TerrainProfile {
    int octaves;
    float frequency;
    float amplitude;
    float lacunarity;
    float persistence;
};

constexpr TerrainProfile PLAINS {
    4, 0.015f, 10.0f, 2.0f, 0.4f
};

constexpr TerrainProfile HILLS {
    4, 0.02f, 18.0f, 2.0f, 0.45f
};

constexpr TerrainProfile MOUNTAINS {
    5, 0.01f, 65.0f, 2.0f, 0.55f
};

Biome SelectBiome(float temperature){
    if (temperature < 0.4f) return Biome::Rockland;   // cold
    if (temperature >= 0.4f && temperature < 0.6f) return Biome::Grassland;  // temperate
    return Biome::Desert; 
}

Landform SelectLandform(float erosion){
    if (erosion < 0.33f)
        return Landform::Mountains;

    if (erosion < 0.66f)
        return Landform::Hills;

    return Landform::Plains;
}

const TerrainProfile& GetProfile(Landform landform){
    switch (landform) {
        case Landform::Plains:  return PLAINS;
        case Landform::Hills:  return HILLS;
        case Landform::Mountains:  return MOUNTAINS;
    }

    assert(false && "Unexpected Lanform value");

    return PLAINS;
}

int GetSurfaceHeight(
    int worldX,
    int worldZ,
    const TerrainProfile& profile
) {
    float frequency = profile.frequency;
    float octaveAmplitude = 1.0f;

    float totalNoise = 0.0f;
    float totalAmplitude = 0.0f;

    const glm::vec2 pos(
        static_cast<float>(worldX),
        static_cast<float>(worldZ)
    );

    for (int i = 0; i < profile.octaves; ++i) {
        totalNoise += glm::perlin(pos * frequency) * octaveAmplitude;
        totalAmplitude += octaveAmplitude;

        frequency *= profile.lacunarity;
        octaveAmplitude *= profile.persistence;
    }

    // Normalize noise to approximately -1.0 to 1.0.
    const float normalizedNoise = totalNoise / totalAmplitude;

    // Apply overall terrain height.
    const float height = SURFACE_Y + normalizedNoise * profile.amplitude;

    return std::clamp(
        static_cast<int>(std::round(height)),
        1,
        CHUNK_HEIGHT - 1
    );
}

} // namespace end

TerrainColumn TerrainGenerator::GenerateColumn(
    int worldX,
    int worldZ
) const {

    // Sample biome noise.
    const glm::vec2 pos(
        static_cast<float>(worldX),
        static_cast<float>(worldZ)
    );

    float erosion = glm::perlin(
        pos * 0.001f + glm::vec2(37.2f, 91.7f)
    ) * 0.5f + 0.5f;

    float temperature = glm::perlin(
        pos * 0.001f + glm::vec2(143.8f, 57.3f)
    ) * 0.5f + 0.5f;

    // Select biome and landform independently.
    Biome biome = SelectBiome(temperature);
    Landform landform = SelectLandform(erosion);

    // Generate surface height.
    const TerrainProfile& profile = GetProfile(landform);

    int surfaceY = GetSurfaceHeight(
        worldX, worldZ, profile
    );

    // fill in the struct
    TerrainColumn column;
    column.biome = biome;
    column.landform = landform;
    column.surfaceY = surfaceY;

    // LOG_DEBUG("Current Biome: " + static_cast<int>(biome));

    // Generate blocks according to biome.
    for (int y = 0; y < CHUNK_HEIGHT; ++y) {

        if (y > surfaceY) {
            column.blocks[y] = BlockType::Air;
        }
        else {
            switch (biome) {

                case Biome::Desert:
                    if (y > surfaceY - 4)
                        column.blocks[y] = BlockType::Sand;
                    else
                        column.blocks[y] = BlockType::Stone;
                    break;

                case Biome::Grassland:
                    if (y == surfaceY)
                        column.blocks[y] = BlockType::Grass;
                    else if (y > surfaceY - 4)
                        column.blocks[y] = BlockType::Dirt;
                    else
                        column.blocks[y] = BlockType::Stone;
                    break;
                
                case Biome::Rockland:
                    if (y == surfaceY)
                        column.blocks[y] = BlockType::Stone;
                    else if (y > surfaceY - 4)
                        column.blocks[y] = BlockType::Dirt;
                    else
                        column.blocks[y] = BlockType::Stone;
                    break;
            }
        }
    }

    return column;
}