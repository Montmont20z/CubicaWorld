#include "CharacterController.hpp"
#include "Camera.hpp"
#include "ChunkManager.hpp"
#include "Player.hpp"
#include <cmath>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>


namespace  {

constexpr float COLLISION_EPSILON = 0.000001f;

// Get the range of integer voxel coordinates that actually
// overlap an AABB. The maximum uses nextafter to exclude a
// voxel that merely touches the AABB's maximum face.
int MinVoxel(float value){
    return static_cast<int>(std::floor(value));
}

int MaxVoxel(float value) {
    return static_cast<int>(
        std::floor(std::nextafter( // for overflow safety
            value,
            -std::numeric_limits<float>::infinity()
        ))
    );
}

} // namespace


// --------------------------------------------------
// Check whether all occupied voxel cells are available.
// --------------------------------------------------
bool CharacterController::IsBoundsKnown(
    const AABB& bounds, 
    const ChunkManager& world
) const {
    const int minX = MinVoxel(bounds.min.x);
    const int maxX = MaxVoxel(bounds.max.x);
    
    const int minY = MinVoxel(bounds.min.y);
    const int maxY = MaxVoxel(bounds.max.y);

    const int minZ = MinVoxel(bounds.min.z);
    const int maxZ = MaxVoxel(bounds.max.z);
    

    for (int x = minX; x <= maxX; ++x) {
        for (int y = minY; y <= maxY; ++y) {
            for (int z = minZ; z <= maxZ; ++z) {

                const auto result = world.QueryBlock({x, y, z});

                if (result == BlockQueryResult::Unknown)
                    return false;
            }
        }
    }

    return true;
}

// --------------------------------------------------
// Move along one axis and resolve AABB collisions. // decide whether a movement is legal/illegal
// --------------------------------------------------
bool CharacterController::MoveAxis(
    Player& player,
    const ChunkManager& world,
    int axis, // index into glm::vec3 as X,Y,Z
    float displacement
) const {
    if (std::abs(displacement) < COLLISION_EPSILON)
        return false;
    
    glm::vec3 newPosition = player.GetPosition();
    newPosition[axis] += displacement;

    // Temporarily apply the proposed movement.
    player.SetPosition(newPosition);

    const AABB bounds = player.GetBounds();

    const int minX = MinVoxel(bounds.min.x);
    const int maxX = MaxVoxel(bounds.max.x);

    const int minY = MinVoxel(bounds.min.y);
    const int maxY = MaxVoxel(bounds.max.y);

    const int minZ = MinVoxel(bounds.min.z);
    const int maxZ = MaxVoxel(bounds.max.z);
    
    float resolvedPosition = newPosition[axis];
    bool collided = false;
    
    // Player bounding-box offsets relative to its feet position.
    const float halfWidth = player.GetWidth() * 0.5f;
    
    const float minOffset = axis == 1 ? 0.0f : -halfWidth;
    const float maxOffset = axis == 1 ? player.GetHeight() : halfWidth;
    
     for (int x = minX; x <= maxX; ++x) {
        for (int y = minY; y <= maxY; ++y) {
            for (int z = minZ; z <= maxZ; ++z) {
                const BlockQueryResult result = world.QueryBlock({x,y,z});
                
                // Air does not collide.
                if (result == BlockQueryResult::Air)
                    continue;
                // Unknown, Solid is treated as solid.
                
                const AABB blockBounds(
                    glm::vec3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)),
                    glm::vec3(static_cast<float>(x+1), static_cast<float>(y+1), static_cast<float>(z+1))
                );
                
                // no collision
                if (!bounds.Intersects(blockBounds))
                    continue;

                // collision detected
                collided = true;
                
                const float blockMin = 
                    axis == 0 ? static_cast<float>(x) :
                    axis == 1 ? static_cast<float>(y) :
                                static_cast<float>(z);
                
                const float blockMax = blockMin + 1.0f;

                if (displacement > 0.0f) {
                    // Moving toward the positive axis.
                    // Clamp the player's maximum face to block minimum.
                    resolvedPosition = std::min(
                        resolvedPosition,
                        blockMax - maxOffset
                    );
                } else {
                    // Moving toward the negative axis.
                    // Clamp the player's minimum face to block maximum.
                    resolvedPosition = std::max(
                        resolvedPosition,
                        blockMax - minOffset
                    );
                }
            }
        }    
    }
     
    // Apply the collision-resolved position
    newPosition[axis] = resolvedPosition;
    player.SetPosition(newPosition);

    return collided;
}

// --------------------------------------------------
// Main Character Controller Update
// --------------------------------------------------
void CharacterController::Update(
    Player& player,
    const PlayerInput& input,
    const Camera& camera,
    const ChunkManager& world,
    float deltaTime
) {
    // Avoid large physics steps after a frame hitch.
    deltaTime = std::clamp(deltaTime, 0.0f, 0.05f);

    if (deltaTime <= 0.0f)
        return;
    
    // Wait for the player's current chunk data to be available
    // This avoids simulating physics against unfinished terrain.
    if (!IsBoundsKnown(player.GetBounds(), world))
        return;
    
    // --------------------------------------------------
    // 1. Calculate horizontal movement direction.
    // --------------------------------------------------
    glm::vec3 forward = camera.GetFront();
    forward.y = 0.0f;
    
    forward = glm::normalize(forward);

    const glm::vec3 worldUp(0.0f,1.0f,0.0f);
    glm::vec3 right = glm::normalize(glm::cross(forward, worldUp));

    glm::vec3 movement = forward * input.forward + right * input.right;

    const float speed = input.sprintHeld ? SPRINT_SPEED : WALK_SPEED;
    // --------------------------------------------------
    // 2. Horizontal movement.
    // --------------------------------------------------
    glm::vec3 velocity = player.GetVelocity();
    
    velocity.x = movement.x * speed;
    velocity.z = movement.z * speed;
    
    if (MoveAxis(player, world, 0, velocity.x * deltaTime))
        velocity.x = 0.0f;

    if (MoveAxis(player, world, 2, velocity.z * deltaTime))
        velocity.z = 0.0f;

    // --------------------------------------------------
    // 3. Jumping.
    // --------------------------------------------------
    const bool wasGrounded = player.IsGrounded();
    
    if (input.jumpPressed ){
        velocity.y = JUMP_SPEED;
    } else if (wasGrounded && velocity.y < 0.0f){
        velocity.y = 0.0f;
    }
    // Reset grounded state before vertical movement.
    // It will be restored if we collide while falling.
    player.SetGrounded(false);

    // --------------------------------------------------
    // 4. Gravity.
    // --------------------------------------------------
    velocity.y -= GRAVITY * deltaTime;
    velocity.y = std::max(velocity.y, -TERMINAL_VELOCITY);
    
    // --------------------------------------------------
    // 5. Vertical movement and collision.
    // --------------------------------------------------
    const float verticalMovement = velocity.y * deltaTime;
    
    const bool verticalCollision = MoveAxis(player, world, 1, verticalMovement);

    if (verticalCollision){
        // Landing on a block.
        if (verticalMovement < 0.0f)
            player.SetGrounded(true);

        // Hitting either floor or ceiling.
        velocity.y = 0.0f;
    }

    // --------------------------------------------------
    // 6. Save velocity.
    // --------------------------------------------------
    player.SetVelocity(velocity);
}