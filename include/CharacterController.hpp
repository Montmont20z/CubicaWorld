#pragma once
#include "ChunkManager.hpp"
#include "Player.hpp"

struct PlayerInput {
    float forward = 0.0f;
    float right = 0.0f;
    bool jumpPressed = false;
    bool sprintHeld = false;
};

class CharacterController {
public:
    void Update(
        Player& player,
        const PlayerInput& input,
        const Camera& camera,
        const ChunkManager& world,
        float deltaTime
    );

private:
    // Move along one axis and resolve collisions.
    // axis: 0 = X, 1 = Y, 2 = Z    
    bool MoveAxis(
        Player& player,
        const ChunkManager& world,
        int axis,
        float displacement
    ) const;
    
    // Ensure the player's current region has finished generating
    bool IsBoundsKnown(
        const AABB& bounds,
        const ChunkManager& world
    )const;

    static constexpr float WALK_SPEED = 10.0f;
    static constexpr float SPRINT_SPEED = 20.0f;

    static constexpr float GRAVITY = 14.0f;
    static constexpr float JUMP_SPEED = 8.5f;
    static constexpr float TERMINAL_VELOCITY = 55.0f;    

    void ApplyGravity(Player& player, float deltaTime);

    void MoveAndCollide(
        Player& player,
        ChunkManager& world,
        float deltaTime
    );
};