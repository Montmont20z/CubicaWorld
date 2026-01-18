#include "Camera.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>


Camera::Camera(int width, int height, glm::vec3 position)
    : ScreenWidth(width)
    , ScreenHeight(height)
    , Position(position)
{
}

void Camera::UpdateMatrix(float FOVdeg, float nearPlane, float farPlane)
{
    if (ScreenHeight == 0) return; // guard

    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);
    
    view = glm::lookAt(Position, Position + Front, Up);
    float aspect = static_cast<float>(ScreenWidth) / static_cast<float>(ScreenHeight);
    projection = glm::perspective(glm::radians(FOVdeg), aspect, nearPlane, farPlane);
    
    cameraMatrix_ = projection * view;
}

void Camera::SetUniformMatrix(Shader& shader, const std::string& uniform){
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, uniform.c_str()), 1, GL_FALSE, glm::value_ptr(cameraMatrix_));
}

void Camera::ProcessInput(GLFWwindow *window)
{
    glm::vec3 right = glm::normalize(glm::cross(Front, Up));

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS){
        glfwSetWindowShouldClose(window, true);
    }
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS){
       Position += Front * MoveSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS){
       Position -= Front * MoveSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS){
       Position -= right * MoveSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS){
       Position += right * MoveSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS){
       Position += Up * MoveSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS){
       Position -= Up * MoveSpeed;
    }
    // if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS){
    // //    Position += Up * MoveSpeed;
	// 	MoveSpeed = 0.2f;
    // }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
	{
		MoveSpeed = 0.2f;
	}
	else if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_RELEASE)
	{
		MoveSpeed = 0.05f;
	}
    
    // handle mouse movement
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
	{
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);    
        
        // Prevents camera from jumping on the first click
		if (isFirstClick_)
		{
			glfwSetCursorPos(window, (ScreenWidth / 2), (ScreenHeight / 2));
			isFirstClick_ = false;
		}
        
        double mouseX;
        double mouseY;
        // Fetches the coordinates of the cursor 
        glfwGetCursorPos(window, &mouseX, &mouseY);
        
        float rotY = MouseSensitivity * static_cast<float>((mouseY - (ScreenHeight / 2)) / ScreenHeight);
        float rotX = MouseSensitivity * static_cast<float>((mouseX - (ScreenWidth / 2)) / ScreenWidth);
        
        // Calculate upcoming vertical change in the Orientation
        glm::vec3 newFront = glm::rotate(Front, glm::radians(-rotY), right);
        
        // Decides whether or not the next vertical Orientation is legal or not
		if (abs(glm::angle(newFront, Up) - glm::radians(90.0f)) <= glm::radians(85.0f))
		{
            Front = glm::normalize(newFront);
        }

        // Rotate Front around the Up vector
        Front = glm::normalize(glm::rotate(Front, glm::radians(-rotX), Up));
        
        // Sets mouse cursor to the middle of the screen, enable infinite scrolling 
        glfwSetCursorPos(window, (ScreenWidth / 2), (ScreenHeight / 2));
	} else {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        isFirstClick_ = true;
    }
}

void Camera::SetViewportSize(int width, int height)
{
    ScreenWidth = width;
    ScreenHeight = height;
}
