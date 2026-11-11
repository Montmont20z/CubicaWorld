#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <math.h>
#include <stb/stb_image.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Logger.hpp"
#include "ChunkManager.hpp"
#include "Shader.hpp"
#include "Texture.hpp"
#include "Camera.hpp"
#include "Mesh.hpp"
#include "RenderSettings.hpp"

int screenWidth = 1200;
int screenHeight = 1200;

void frameBufferCallback(GLFWwindow* window, int width, int height){
    glViewport(0,0, width, height);
    
    // Update Camera stored in the window user pointer
    Camera* cam = static_cast<Camera*>(glfwGetWindowUserPointer(window));
    if (cam) {
        cam->SetViewportSize(width, height);
    }
}

static void errorCallback(int error, const char* description){
    LOG_ERROR("GLFW error " + std::to_string(error) + ": " + description);
}

#ifdef DEBUG_MODE
    void APIENTRY OpenGLDebugCallback(
        GLenum source,
        GLenum type,
        GLuint id,
        GLenum severity,
        GLsizei length,
        const GLchar* message,
        const void* userParam
    ) {
        LogLevel level;

        switch (severity) {
            case GL_DEBUG_SEVERITY_HIGH:
                level = LogLevel::Error;
                break;

            case GL_DEBUG_SEVERITY_MEDIUM:
                level = LogLevel::Warning;
                break;

            case GL_DEBUG_SEVERITY_LOW:
                level = LogLevel::Debug;
                break;

            case GL_DEBUG_SEVERITY_NOTIFICATION:
                return; // Ignore informational notifications

            default:
                level = LogLevel::Warning;
                break;
        }

        std::string logMessage =
            "OpenGL [" + std::to_string(id) + "] " +
            std::string(message, length);

        Logger::Log(level, logMessage);
    }
#endif

int main(){
    /* Initialization */
    Logger::Init("logs/CubicaWorld.log");
    LOG_INFO("Application starting");
    
    glfwSetErrorCallback(errorCallback);
    /* Initialize the library (glfw3) */
    if (!glfwInit()) {        
        LOG_ERROR("Failed to initialize GLFW");
        glfwTerminate();
        Logger::Shutdown();
        return -1;
    }
    // set OpenGL version (4.3 Core)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // do not use deprecated functions (eg glBegin, glEnd)
    #ifdef DEBUG_MODE
        LOG_DEBUG("Debug Mode Enabled");
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    #endif

    // create the window
    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "Cubica World", nullptr, nullptr);
    if (window == nullptr){
        LOG_ERROR("Failed to create GLFW window");
        glfwDestroyWindow(window);
        glfwTerminate();
        Logger::Shutdown();
        return -1;
    }
    glfwMakeContextCurrent(window);
    LOG_INFO("GLFW window initialized");

    // load OpenGL function pointers using GLAD // must be loaded at runtime
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        LOG_ERROR("Failed to initialize GLAD");
        glfwDestroyWindow(window);
        glfwTerminate();
        Logger::Shutdown();
        return -1;
    }
    LOG_INFO("OpenGL initialized");
    
    glfwSwapInterval(1); // Enable V-sync

    glViewport(0,0, screenWidth, screenHeight); // Maps normalized device coordinates (NDC, range [-1,1]) to window pixels. // need to change Screen width & height if resize window
    
    Shader chunkShader("shaders/chunk.vert.glsl", "shaders/chunk.frag.glsl");
    
    ChunkManager chunkManager(VIEW_DISTANCE, IS_SINGLE_THREADED);

    glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    glm::vec3 lightPos = glm::vec3(1.5f, 0.5f, 0.0f);
    glm::mat4 lightModel = glm::mat4(1.0f);
    lightModel = glm::translate(lightModel, lightPos);

    // Texture
    std::unique_ptr textureAtlas = std::make_unique<Texture>("assets/DefaultPack.png", GL_TEXTURE_2D, GL_TEXTURE0, GL_UNSIGNED_BYTE);
    textureAtlas->Bind(0);
    chunkShader.Activate();
    glUniform1i(glGetUniformLocation(chunkShader.ID, "atlasTiles"), 16);
    chunkShader.AttachTextureUnit(0, "tex0");
    LOG_INFO("Loaded Texture Atlas");
    
    Camera camera(screenWidth, screenHeight, glm::vec3(0.0f, 68.0f, 0.0f));
    // set user pointer and callback
    glfwSetWindowUserPointer(window, &camera);
    glfwSetFramebufferSizeCallback(window, frameBufferCallback);
    
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    
    float prevTime = static_cast<float>(glfwGetTime());
    
    // OpenGL Debug
    #ifdef DEBUG_MODE
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback( OpenGLDebugCallback , nullptr);
    #endif

    // main game loop
    while(!glfwWindowShouldClose(window)){
        /* check game state -> input -> update -> render */

        // delta time
        float now = static_cast<float>(glfwGetTime());
        float delta = now - prevTime;
        prevTime = now;

        // process input
        glfwPollEvents();
        camera.ProcessInput(window);
        
        // render
        glClearColor(0.53f, 0.82f, 0.98f, 1.0f); // sky blue
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        camera.UpdateMatrix(45.0f, 0.1f, 500.0f);
        
        // activate shader for chunkManager update
        chunkShader.Activate();
        chunkShader.SetVec3("lightPos", lightPos);
        chunkShader.SetVec4("lightColor", lightColor);
        
        // Loads/unload chunks, drains upload queue, draw everything
        chunkManager.Update(camera.Position, chunkShader, camera);

        // swap buffers
        glfwSwapBuffers(window);
    }

    // clean up resources
    LOG_INFO("Application shutting down");
    Logger::Shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}