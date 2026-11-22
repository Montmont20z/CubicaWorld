#pragma once

#include "Frustum.hpp"
#include <glad/glad.h>
#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <Shader.hpp>
#include <string>
#include <GLFW/glfw3.h>

class Camera {
public:
    Camera(int width, int height, glm::vec3 position);
    
     // Camera position.
    void SetPosition(const glm::vec3& position) noexcept;
    const glm::vec3& GetPosition() const noexcept;

    // Camera orientation.
    const glm::vec3& GetFront() const noexcept;
    const glm::vec3& GetRight() const noexcept;
    const glm::vec3& GetUp() const noexcept;
    const glm::vec3& GetWorldUp() const noexcept;
    
    // Mouse look only. Does not control cursor mode.
    void ProcessMouseLook(GLFWwindow* window, bool enabled);

    // View and projection.
    void UpdateMatrix(float FOVdeg, float nearPlane, float farPlane);
    void SetUniformMatrix(const Shader &shader, const std::string &uniform) const;

    void SetViewportSize(int width, int height);
    void SetMouseSensitivity(float sensitivity) noexcept;
    const glm::mat4& GetCameraMatrix() const noexcept;
    Frustum GetFrustum() const { return Frustum(cameraMatrix_); }
    
    
private:
    void UpdateCameraVectors();    

    glm::vec3 position_;
    glm::vec3 front_{glm::vec3(0.0f, 0.0f, -1.0f)};
    glm::vec3 right_{1.0f, 0.0f, 0.0f};
    glm::vec3 up_{glm::vec3(0.0f, 1.0f, 0.0f)};
    glm::vec3 worldUp_{0.0f, 1.0f, 0.0f};
    
    float yaw_ = -90.0f;
    float pitch_ = 0.0f;

    int screenWidth_;
    int screenHeight_;

    float moveSpeed_{0.1f};
    float mouseSensitivity_{0.1f};
    
    bool firstMouse_ = true;
    
    double lastMouseX_ = 0.0;
    double lastMouseY_ = 0.0;

    glm::mat4 cameraMatrix_{1.0f};
};