#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <math.h>
#include <stb/stb_image.h>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.hpp"
#include "VAO.hpp"
#include "VBO.hpp"
#include "EBO.hpp"
#include "Texture.hpp"
#include "main.h"

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 800;

static void errorCallback(int error, const char* description){
    fprintf(stderr, "Error: %s\n", description);
}

static void processInput(GLFWwindow* window){
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS){
        glfwSetWindowShouldClose(window, true);
    }
}

int main(){
    glfwSetErrorCallback(errorCallback);
    std::cout << "Hello, World!3333" << std::endl;
    
    /* Initialization */
    /* Initialize the library (glfw3) */
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    // set OpenGL version (3.3 Core)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // do not use deprecated functions (eg glBegin, glEnd)
    // remove this after development
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    
    GLfloat vertices[] = {
        //  Position ------// --- Color ---------// ----- Texture
        -0.5f, 0.0f,  0.5f,    1.0f, 0.0f, 0.0f,    0.0f, 0.0f, 
        -0.5f, 0.0f, -0.5f,     0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
        0.5f,  0.0f, -0.5f,     0.0f, 0.0f, 1.0f,    0.0f, 0.0f,
        0.5f,  0.0f,  0.5f,    1.0f, 1.0f, 1.0f,     1.0f, 0.0f,
        0.0f,  0.8f,  0.0f,    1.0f, 1.0f, 1.0f,     0.5f, 1.0f, // pyramid top corner
    };
    
    unsigned int vertexIndices[] = {
        0,1,2, // bottom 1
        2,3,0, // bottom 2
        0,1,4,
        1,2,4,
        2,3,4,
        3,0,4,
    };
    
    // create the window
    GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Cubica World", nullptr, nullptr);
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

    glViewport(0,0, SCREEN_WIDTH, SCREEN_HEIGHT); // Maps normalized device coordinates (NDC, range [-1,1]) to window pixels. // need to change Screen width & height if resize window
    
    Shader shader("shaders/basic.vert.glsl", "shaders/basic.frag.glsl");
    
    
    // VAO (Vertex Array Object), VBO (Vertex Buffer Object), EBO (Element Buffer Object) or Index buffer
    VAO VAO1;
    VBO VBO1(vertices, sizeof(vertices));

    VAO1.Bind();
    VAO1.LinkAttrib(VBO1, 0, 3, GL_FLOAT, 8 * sizeof(float), (void*)0);
    VAO1.LinkAttrib(VBO1, 1, 3, GL_FLOAT, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    VAO1.LinkAttrib(VBO1, 2, 2, GL_FLOAT, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    
    // create EBO while VAO is bound so VAO stores element array binding
    EBO EBO1(vertexIndices, sizeof(vertexIndices));
    EBO1.Bind();
    
    // unbind VAO, VBO, EBO
    VBO1.Unbind();
    VAO1.Unbind();
    EBO1.Unbind();
    
    GLuint uniID = glGetUniformLocation(shader.ID, "scale");
    
    // Texture
    Texture popCat("assets/pop-cat.jpg", GL_TEXTURE_2D, GL_TEXTURE0, GL_RGB, GL_UNSIGNED_BYTE);
    popCat.texUnit(shader, "tex0", 0);

    float rotation = 0.0f;
    float prevTime = glfwGetTime();
    
    glEnable(GL_DEPTH_TEST);

    // main game loop
    while(!glfwWindowShouldClose(window)){
        /* check game state -> input -> update -> render */
        // process input
        glfwPollEvents();
        processInput(window);
        
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
        glm::mat4 view = glm::mat4(1.0f);
        glm::mat4 proj = glm::mat4(1.0f);
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(1.0f,0.0f,0.0f));
        view = glm::translate(view, glm::vec3(0.0f,-0.5f,-2.0f));
        proj = glm::perspective(glm::radians(45.0f), static_cast<float>(SCREEN_WIDTH/SCREEN_HEIGHT), 0.1f, 100.0f); // Fov, aspect ratio, near field, far field
                                                                                                                    
        int modelLoc = glGetUniformLocation(shader.ID, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        int viewLoc = glGetUniformLocation(shader.ID, "view");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        int projLoc = glGetUniformLocation(shader.ID, "proj");
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(proj));

        glUniform1f(uniID, 1.0f); // set scale to 1x on runtime
        popCat.Bind(0);
        VAO1.Bind();
        
        // glDrawArrays(GL_TRIANGLES, 0, 3); // draw triangle
        glDrawElements(GL_TRIANGLES, sizeof(vertexIndices)/sizeof(int), GL_UNSIGNED_INT, 0); 

        // swap buffers
        glfwSwapBuffers(window);
    }

    // clean up resources
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}




