#pragma once
#include <glm/exponential.hpp>
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>
#include "AABB.hpp"

class Player {
public:
    static constexpr float DEFAULT_WIDTH = 0.6f;
    static constexpr float DEFAULT_HEIGHT = 1.8f;
    static constexpr float DEFAULT_EYE_HEIGHT = 1.62f;
    
    explicit Player(
        const glm::vec3& spawnPosition = glm::vec3(0.0),
        float width = DEFAULT_WIDTH,
        float height = DEFAULT_HEIGHT,
        float eyeHeight = DEFAULT_EYE_HEIGHT 
    );
    
    // Position represents the player's feet, centered horizontally.
    const glm::vec3& GetPosition() const noexcept;
    void SetPosition(const glm::vec3& position) noexcept;
    
    const glm::vec3& GetVelocity() const noexcept;
    void SetVelocity(const glm::vec3& velocity) noexcept;

    bool IsGrounded() const noexcept;
    void SetGrounded(bool grounded) noexcept;
    
    float GetWidth() const noexcept;
    float GetHeight() const noexcept;
    float GetEyeHeight() const noexcept;

    // Physical bounding box.
    AABB GetBounds() const;

    // Camera position relative to the player's feet.
    glm::vec3 GetEyePosition() const;

private:
    glm::vec3 position_{0.0f};
    glm::vec3 velocity_{0.0f};

    float width_;
    float height_;
    float eyeHeight_;

    bool grounded_ = false;



};