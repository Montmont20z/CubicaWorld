#pragma once
#include <glm/vec3.hpp>
// Axis-aligned bounding box in WORLD space
struct AABB {
    glm::vec3 min;
    glm::vec3 max;
};
