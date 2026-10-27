#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <math.h>
#include <stb/stb_image.h>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "ChunkManager.hpp"
#include "Shader.hpp"
#include "Texture.hpp"
#include "Camera.hpp"
#include "Mesh.hpp"

int screenWidth = 800;
int screenHeight = 800;

void frameBufferCallback(GLFWwindow* window, int width, int height){
    glViewport(0,0, width, height);
    
    // Update Camera stored in the window user pointer
    Camera* cam = static_cast<Camera*>(glfwGetWindowUserPointer(window));
    if (cam) {
        cam->SetViewportSize(width, height);
    }
}

static void errorCallback(int error, const char* description){
    fprintf(stderr, "Error: %s\n", description);
}

int main(){
    // std::cout << "Hello, World!3333" << std::endl;
    
    /* Initialization */
    /* Initialize the library (glfw3) */
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    glfwSetErrorCallback(errorCallback);
    // set OpenGL version (3.3 Core)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // do not use deprecated functions (eg glBegin, glEnd)
    // TODO: remove this after development
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

    std::vector<Vertex> lightVertices = {
        // Position              Normal (unused)       Color (unused)        TexCoord (unused)
        {{-0.1f, -0.1f,  0.1f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
        {{-0.1f, -0.1f, -0.1f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
        {{ 0.1f, -0.1f, -0.1f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
        {{ 0.1f, -0.1f,  0.1f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
        {{-0.1f,  0.1f,  0.1f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
        {{-0.1f,  0.1f, -0.1f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
        {{ 0.1f,  0.1f, -0.1f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
        {{ 0.1f,  0.1f,  0.1f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
    };
    
    std::vector<GLuint> lightIndices = {
        0, 1, 2,
        0, 2, 3,
        0, 4, 7,
        3, 7, 6,
        3, 6, 2,
        2, 6, 5,
        2, 5, 1,
        1, 5, 4,
        1, 4, 0,
        4, 5, 6,
        4, 6, 7,
    };
    
    // create texture array
    // std::vector<std::unique_ptr<Texture>> textures;
    std::vector<std::unique_ptr<Texture>> emptyTextures; // No textures for light

    // create the window
    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "Cubica World", nullptr, nullptr);
    if (window == nullptr){
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // load OpenGL function pointers using GLAD // must be loaded at runtime
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }
    
    glfwSwapInterval(1); // Enable V-sync

    glViewport(0,0, screenWidth, screenHeight); // Maps normalized device coordinates (NDC, range [-1,1]) to window pixels. // need to change Screen width & height if resize window
    
    // Shader blockShader("shaders/basic.vert.glsl", "shaders/basic.frag.glsl");
    // Shader lightShader("shaders/light.vert.glsl", "shaders/light.frag.glsl");
    Shader chunkShader("shaders/chunk.vert.glsl", "shaders/chunk.frag.glsl");
    
    ChunkManager chunkManager(8, false);

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
    
    Camera camera(screenWidth, screenHeight, glm::vec3(1.0f, 68.0f, -1.0f));
    // set user pointer and callback
    glfwSetWindowUserPointer(window, &camera);
    glfwSetFramebufferSizeCallback(window, frameBufferCallback);
    
    // Mesh cubeMesh(vertices, vertexIndices, std::move(textures), GL_STATIC_DRAW);
    Mesh lightMesh(lightVertices, lightIndices, std::move(emptyTextures), GL_STATIC_DRAW);
    
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    
    float prevTime = static_cast<float>(glfwGetTime());
    
    // OpenGL Debug
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback([](GLenum, GLenum, GLuint, GLenum severity, GLsizei, const GLchar* msg, const void*){
        std::cerr << "GL DEBUG: " << msg << std::endl;
    }, nullptr);

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
        // blockShader.Activate();
        // blockShader.SetVec3("lightPos", lightPos);
        // blockShader.SetVec4("lightColor", lightColor);
        // blockShader.SetVec3("camPos", camera.Position);
        // camera.SetUniformMatrix(blockShader, "camMatrix");

        // cubeMesh.Draw(blockShader, camera);
        
        // lightShader.Activate();
        // lightShader.SetMat4("model", lightModel);
        // lightShader.SetVec4("lightColor", lightColor);
        // camera.SetUniformMatrix(lightShader, "camMatrix");
        // lightMesh.Draw(lightShader, camera);
        
        // Loads/unload chunks, drains upload queue, draw everything
        chunkManager.Update(camera.Position, chunkShader, camera);

        // swap buffers
        glfwSwapBuffers(window);
    }

    // clean up resources
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}