#include "Player.hpp"
#include <algorithm>

Player::Player(
    const glm::vec3& spawnPosition,
    float width,
    float height,
    float eyeHeight
)
    : position_(spawnPosition)
    , width_(std::max(width, 0.01f))
    , height_(std::max(height, 0.01f))
    , eyeHeight_(std::clamp(
        eyeHeight,
        0.0f,
        std::max(height, 0.01f)
    ))
{
}

const glm::vec3& Player::GetPosition() const noexcept {
    return position_;
}

void Player::SetPosition(const glm::vec3& position) noexcept {
    position_ = position;
}

const glm::vec3& Player::GetVelocity() const noexcept {
    return velocity_;
}

void Player::SetVelocity(const glm::vec3& velocity) noexcept {
    velocity_ = velocity;
}

bool Player::IsGrounded() const noexcept {
    return grounded_;
}

void Player::SetGrounded(bool grounded) noexcept {
    grounded_ = grounded;
}

float Player::GetWidth() const noexcept {
    return width_;
}

float Player::GetHeight() const noexcept {
    return height_;
}

float Player::GetEyeHeight() const noexcept {
    return eyeHeight_;
}

AABB Player::GetBounds() const {
    const float halfWidth = width_ * 0.5f;

    return {
        position_ + glm::vec3(-halfWidth, 0.0f, -halfWidth),
        position_ + glm::vec3( halfWidth, height_, halfWidth)
    };
}

glm::vec3 Player::GetEyePosition() const {
    return position_ + glm::vec3(0.0f, eyeHeight_, 0.0f);
}