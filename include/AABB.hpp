#pragma once
#include <glm/glm.hpp>
// Axis-aligned bounding box in WORLD space
struct AABB {
    glm::vec3 min;
    glm::vec3 max;
    
    AABB() = default;

    AABB(const glm::vec3& minimum, const glm::vec3& maximum)
        : min(minimum), max(maximum)
    {
    }
    
    // Ensure minimum coordinates do not exceed maximum coordinates.
    bool IsValid() const {
        return glm::all(glm::lessThanEqual(min, max));
    }
    
    // Strict intersection: touching faces do not count as overlapping.
    bool Intersects(const AABB& other) const {
        if (!IsValid() || !other.IsValid())
            return false;

        return 
            min.x < other.max.x && max.x > other.min.x &&
            min.y < other.max.y && max.y > other.min.y &&
            min.z < other.max.z && max.z > other.min.z;
    }

    // Check whether a point is inside or on the boundary.
    bool Contains(const glm::vec3& point)const {
        return
            point.x >= min.x && point.x <= max.x &&
            point.y >= min.y && point.y <= max.y &&
            point.z >= min.z && point.z <= max.z;
    }
    
    glm::vec3 GetSize() const {
        return max - min;
    }
    
    glm::vec3 GetCenter() const {
        return (min + max) * 0.5f;
    }
    
    AABB Translated(const glm::vec3& offset) const {
        return AABB(min + offset, max + offset);
    }


};
