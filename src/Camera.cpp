#include "Camera.hpp"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>


Camera::Camera(int width, int height, glm::vec3 position)
    : screenWidth_(width)
    , screenHeight_(height)
    , position_(position)
{
}

// --------------------------------------------------
// Position
// --------------------------------------------------

void Camera::SetPosition(const glm::vec3& position) noexcept {
    position_ = position;
}

const glm::vec3& Camera::GetPosition() const noexcept {
    return position_;
}

// --------------------------------------------------
// Orientation
// --------------------------------------------------

const glm::vec3& Camera::GetFront() const noexcept {
    return front_;
}

const glm::vec3& Camera::GetRight() const noexcept {
    return right_;
}

const glm::vec3& Camera::GetUp() const noexcept {
    return up_;
}

const glm::vec3& Camera::GetWorldUp() const noexcept {
    return worldUp_;
}

// --------------------------------------------------
// Mouse Look
// --------------------------------------------------
void Camera::ProcessMouseLook(GLFWwindow* window, bool enabled) {
    if (!window || !enabled) {
        firstMouse_ = true;
        return;
    }

    double mouseX;
    double mouseY;

    glfwGetCursorPos(window, &mouseX, &mouseY);

    // Prevent camera jumping when mouse look is first activated.
    if (firstMouse_) {
        lastMouseX_ = mouseX;
        lastMouseY_ = mouseY;
        firstMouse_ = false;
        return;
    }

    float offsetX = static_cast<float>(mouseX - lastMouseX_);
    float offsetY = static_cast<float>(lastMouseY_ - mouseY);

    lastMouseX_ = mouseX;
    lastMouseY_ = mouseY;

    yaw_ += offsetX * mouseSensitivity_;
    pitch_ += offsetY * mouseSensitivity_;

    // Prevent camera flipping when looking too far vertically.
    pitch_ = glm::clamp(pitch_, -89.0f, 89.0f);

    UpdateCameraVectors();
}

// --------------------------------------------------
// Update Orientation Vectors
// --------------------------------------------------
void Camera::UpdateCameraVectors() {
    glm::vec3 direction;

    direction.x =
        cos(glm::radians(yaw_)) *
        cos(glm::radians(pitch_));

    direction.y =
        sin(glm::radians(pitch_));

    direction.z =
        sin(glm::radians(yaw_)) *
        cos(glm::radians(pitch_));

    front_ = glm::normalize(direction);

    right_ = glm::normalize(
        glm::cross(front_, worldUp_)
    );

    up_ = glm::normalize(
        glm::cross(right_, front_)
    );
}

void Camera::UpdateMatrix(float FOVdeg, float nearPlane, float farPlane)
{
    if (screenHeight_ <= 0 || screenWidth_ <= 0) return; // guard

    glm::mat4 view = glm::lookAt(position_, position_ + front_, up_);
    
    float aspect = static_cast<float>(screenWidth_) / static_cast<float>(screenHeight_);
    glm::mat4 projection = glm::perspective(glm::radians(FOVdeg), aspect, nearPlane, farPlane);
    
    cameraMatrix_ = projection * view;
}

void Camera::SetUniformMatrix(const Shader& shader, const std::string& uniform) const
{
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, uniform.c_str()), 1, GL_FALSE, glm::value_ptr(cameraMatrix_));
}


void Camera::SetViewportSize(int width, int height)
{
    screenWidth_ = std::max(width,0);
    screenHeight_ = std::max(height,0);
}

void Camera::SetMouseSensitivity(float sensitivity) noexcept {
    mouseSensitivity_ = std::max(sensitivity, 0.0f);
}

const glm::mat4& Camera::GetCameraMatrix() const noexcept {
    return cameraMatrix_;
}


// void Camera::ProcessInput(GLFWwindow *window)
// {
//     glm::vec3 right = glm::normalize(glm::cross(front_, up_));

//     if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS){
//         glfwSetWindowShouldClose(window, true);
//     }
//     if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS){
//        position_ += front_ * moveSpeed_;
//     }
//     if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS){
//        position_ -= front_ * moveSpeed_;
//     }
//     if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS){
//        position_ -= right * moveSpeed_;
//     }
//     if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS){
//        position_ += right * moveSpeed_;
//     }
//     if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS){
//        position_ += up_ * moveSpeed_;
//     }
//     if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS){
//        position_ -= up_ * moveSpeed_;
//     }
//     // if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS){
//     // //    Position += Up * MoveSpeed;
// 	// 	MoveSpeed = 0.2f;
//     // }
//     if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
// 	{
// 		moveSpeed_ = 2.1f;
// 	}
//     else
// 	{
// 		moveSpeed_ = 0.32f;
// 	}
    
//     // handle mouse movement
//     if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
// 	{
//         glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);    
        
//         // Prevents camera from jumping on the first click
// 		if (isFirstClick_)
// 		{
// 			glfwSetCursorPos(window, (screenWidth_ / 2), (screenHeight_ / 2));
// 			isFirstClick_ = false;
// 		}
        
//         double mouseX;
//         double mouseY;
//         // Fetches the coordinates of the cursor 
//         glfwGetCursorPos(window, &mouseX, &mouseY);
        
//         float rotY = mouseSensitivity_ * static_cast<float>((mouseY - (screenHeight_ / 2)) / screenHeight_);
//         float rotX = mouseSensitivity_ * static_cast<float>((mouseX - (screenWidth_ / 2)) / screenWidth_);
        
//         // Calculate upcoming vertical change in the Orientation
//         glm::vec3 newFront = glm::rotate(front_, glm::radians(-rotY), right);
        
//         // Decides whether or not the next vertical Orientation is legal or not
// 		if (std::abs(glm::angle(newFront, up_) - glm::radians(90.0f)) <= glm::radians(85.0f))
// 		{
//             front_ = glm::normalize(newFront);
//         }

//         // Rotate Front around the Up vector
//         front_ = glm::normalize(glm::rotate(front_, glm::radians(-rotX), up_));
        
//         // Sets mouse cursor to the middle of the screen, enable infinite scrolling 
//         glfwSetCursorPos(window, (screenWidth_ / 2), (screenHeight_ / 2));
// 	} else {
//         glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
//         isFirstClick_ = true;
//     }
// }

