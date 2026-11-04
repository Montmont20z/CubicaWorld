#pragma once
#include "AABB.hpp"
#include <array>
#include <glm/glm.hpp>

class Frustum {
public:
    Frustum() = default;

    explicit Frustum(const glm::mat4& viewProj) {
        // create planes_
        // glm is column-major : m[col][row]. Grab row i as a vec4
        auto row = [&](int i){
            /*     col0  col1  col2  col3
            row0   [0i    1i    2i    3i ]
            row1   |0i    1i    2i    3i |
            row2   |0i    1i    2i    3i |
            row3   [0i    1i    2i    3i ]
                    x     y     z     w       */
            return glm::vec4(viewProj[0][i], viewProj[1][i], viewProj[2][i], viewProj[3][i]);
        };
        
        // Planes represent in Ax + By + Cz + D = 0
        planes_[Left]   = row(3) + row(0);
        planes_[Right]  = row(3) - row(0);
        planes_[Bottom] = row(3) + row(1);
        planes_[Top]    = row(3) - row(1);
        planes_[Near]   = row(3) + row(2); // OpenGL clip space: z in [-w, w]
        planes_[Far]    = row(3) - row(2);

        // Normalise so the plane distance test is in real world units
        for (auto& plane: planes_){
            const float len = glm::length(glm::vec3(plane));
            plane /= len;
        }
    }

    // Returns false only if the box is completely outside at least one plane.
    bool IsVisible(const AABB& aabb){
        // Check if chunk is within view frustum 
        // Loop through all 6 planes
        for (const glm::vec4& plane: planes_){ 
            // "Positive vertex Test ": the box corner furthest along the plane normal.
            // If even that corner is behind the plane, the whole box is outside.
            const glm::vec3 normal(plane);

            const glm::vec3 pv( // choose positive or negative axis to test intersect
                (plane.x >= 0) ? aabb.max.x : aabb.min.x,
                (plane.y >= 0) ? aabb.max.y : aabb.min.y,
                (plane.z >= 0) ? aabb.max.z : aabb.min.z
            );

            // Signed distance from positive vertex to plane.
            float distance = glm::dot(normal, pv) + plane.w;

            // Entire AABB is outside this plane, return false
            if (distance < 0)
                return false;
        }
        return true;
    }

private:
    enum Side {Left, Right, Bottom, Top, Near, Far};
    std::array<glm::vec4, 6> planes_{};
};