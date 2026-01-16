#pragma once

#include <glad/glad.h>
#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <Shader.hpp>
#include <string>
#include <GLFW/glfw3.h>

class Camera {
private:
    bool isFirstClick_;
    glm::mat4 cameraMatrix_;


public:
    glm::vec3 Position;
    glm::vec3 Front{glm::vec3(0.0f, 0.0f, -1.0f)};
    glm::vec3 Up{glm::vec3(0.0f, 1.0f, 0.0f)};
    
    int ScreenWidth;
    int ScreenHeight;

    float MoveSpeed{0.1f};
    float MouseSensitivity{100.0f};

    Camera(int width, int height, glm::vec3 position);

    void UpdateMatrix(float FOVdeg, float nearPlane, float farPlane);
    void SetUniformMatrix(Shader &shader, const std::string &uniform);
    void ProcessInput(GLFWwindow *window);

    void SetViewportSize(int width, int height);
};