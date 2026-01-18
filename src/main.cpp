#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <math.h>
#include <stb/stb_image.h>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.hpp"
#include "VAO.hpp"
#include "VBO.hpp"
#include "EBO.hpp"
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
    std::cout << "Hello, World!3333" << std::endl;
    
    /* Initialization */
    /* Initialize the library (glfw3) */
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    glfwSetErrorCallback(errorCallback);
    // set OpenGL version (3.3 Core)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // do not use deprecated functions (eg glBegin, glEnd)
    // remove this after development
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    
    std::vector<Vertex> vertices = {
        // Position                Normal                  Color                   TexCoords
        {{-0.5f, 0.0f,  0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{-0.5f, 0.0f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{ 0.5f, 0.0f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{ 0.5f, 0.0f,  0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
        {{ 0.0f, 0.8f,  0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {0.5f, 1.0f}},
    };

    // unsigned int vertexIndices[] = {
    std::vector<GLuint> vertexIndices = {
        0,1,2, // bottom 1
        2,3,0, // bottom 2
        0,1,4,
        1,2,4,
        2,3,4,
        3,0,4,
    };
    
    // create texture array
    std::vector<std::unique_ptr<Texture>> textures;

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
    
    // Enable V-sync
    glfwSwapInterval(1);

    glViewport(0,0, screenWidth, screenHeight); // Maps normalized device coordinates (NDC, range [-1,1]) to window pixels. // need to change Screen width & height if resize window
    
    Shader shader("shaders/basic.vert.glsl", "shaders/basic.frag.glsl");
    
    // Texture
    std::unique_ptr popCatTexture = std::make_unique<Texture>("assets/pop-cat.jpg", GL_TEXTURE_2D, GL_TEXTURE0, GL_RGB, GL_UNSIGNED_BYTE);
    popCatTexture->Bind(0);
    shader.AttachTextureUnit(0, "tex0");
    textures.push_back(std::move(popCatTexture));

    float rotation = 0.0f;
    float prevTime = glfwGetTime();
    
    Camera camera(screenWidth, screenHeight, glm::vec3(0.0f, 0.0f, 2.0f));
    // set user pointer and callback
    glfwSetWindowUserPointer(window, &camera);
    glfwSetFramebufferSizeCallback(window, frameBufferCallback);
    
    Mesh mesh(vertices, vertexIndices, std::move(textures), GL_STATIC_DRAW);

    glEnable(GL_DEPTH_TEST);

    // main game loop
    while(!glfwWindowShouldClose(window)){
        /* check game state -> input -> update -> render */
        // process input
        glfwPollEvents();
        // processInput(window);
        camera.ProcessInput(window);
        
        // render
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shader.Activate();
        
        float currentTime = glfwGetTime();
        if (currentTime - prevTime >= 1/60){
            rotation += 0.5f;
            prevTime = currentTime;
        }
        
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(1.0f,0.0f,0.0f));
 
        int modelLoc = glGetUniformLocation(shader.ID, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        camera.UpdateMatrix(45.0f, 0.1f, 100.0f);
        camera.SetUniformMatrix(shader, "camMatrix");
 
        mesh.Draw(shader, camera);

        // swap buffers
        glfwSwapBuffers(window);
    }

    // clean up resources
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}




