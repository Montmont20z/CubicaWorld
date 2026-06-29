#pragma once
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <cstdint>

struct ChunkVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
    uint8_t texLayer; // index into GL_TEXTURE_2D_ARRAY
    uint8_t ao; // 0-3 ambient occlusion level
};
