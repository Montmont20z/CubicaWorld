#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <math.h>
#include "Shader.hpp"
#include "VAO.hpp"
#include "VBO.hpp"
#include "EBO.hpp"

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

int main(){
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
        //  ----------------Position -------------// --- Color ---------//
        -0.5f, -0.5f * float(sqrt(3)) / 3, 0.0f,    0.5f, 0.5f, 0.5f, // lower left
        0.5f, -0.5f * float(sqrt(3)) / 3, 0.0f,     1.0f, 0.5f, 0.2f, // lower right
        0.0f, 0.5f * float(sqrt(3)) * 2 / 3, 0.0f,  1.0f, 0.5f, 0.2f, // upper corner
        -0.5f / 2, 0.5f * float(sqrt(3)) / 6, 0.0f, 0.0f, 0.0f, 0.0f, // inner left
        0.5f / 2, 0.5f * float(sqrt(3)) / 6, 0.0f,  0.0f, 0.0f, 0.0f, // inner right
        0.0f, -0.5f * float(sqrt(3)) / 3, 0.0f,     0.0f, 0.0f, 0.0f, // inner down
    };
    
    unsigned int vertexIndices[] = {
        0, 5, 3, // lower left
        3, 2, 4,
        5, 1, 4,
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
    VAO1.LinkAttrib(VBO1, 0, 3, GL_FLOAT, 6 * sizeof(float), (void*)0);
    VAO1.LinkAttrib(VBO1, 1, 3, GL_FLOAT, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    
    // create EBO while VAO is bound so VAO stores element array binding
    EBO EBO1(vertexIndices, sizeof(vertexIndices));
    EBO1.Bind();
    
    // unbind VAO, VBO, EBO
    VBO1.Unbind();
    VAO1.Unbind();
    EBO1.Unbind();
    
    GLuint uniID = glGetUniformLocation(shader.ID, "scale");

    // main game loop
    while(!glfwWindowShouldClose(window)){
        /* check game state -> input -> update -> render */
        // process input
        glfwPollEvents();
        
        // render
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        shader.Activate();
        glUniform1f(uniID, 0.5f);
        VAO1.Bind();
        
        // glDrawArrays(GL_TRIANGLES, 0, 3); // draw triangle
        glDrawElements(GL_TRIANGLES, 9, GL_UNSIGNED_INT, 0); 

        // swap buffers
        glfwSwapBuffers(window);
    }

    // clean up shader
    // glDeleteVertexArrays(1, &VAO);
    // glDeleteBuffers(1, &VBO);
    // glDeleteBuffers(1, &EBO);
    // glDeleteProgram(shaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}



