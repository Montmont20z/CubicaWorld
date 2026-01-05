#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <math.h>
#include "Shader.hpp"

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
        -0.5f, -0.5f * float(sqrt(3)) / 3, 0.0f, // lower left
        0.5f, -0.5f * float(sqrt(3)) / 3, 0.0f, // lower right
        0.0f, 0.5f * float(sqrt(3)) * 2 / 3, 0.0f, // upper corner
        -0.5f / 2, 0.5f * float(sqrt(3)) / 6, 0.0f, // inner left
        0.5f / 2, 0.5f * float(sqrt(3)) / 6, 0.0f, // inner right
        0.0f, -0.5f * float(sqrt(3)) / 3, 0.0f, // inner down
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
    
    
    GLuint VAO, VBO, EBO; // VAO (Vertex Array Object), VBO (Vertex Buffer Object), EBO (Element Buffer Object) or Index buffer
    // Create object names for VAO and VBO.
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO); 
    glGenBuffers(1, &EBO); 

    glBindVertexArray(VAO); // Bind the VAO. A VAO stores vertex attribute state (which VBOs to use, [x,y,z, nx, ny, nz, u, v, ...] ).
                            
    // upload vertex data (to GPU)
    glBindBuffer(GL_ARRAY_BUFFER, VBO); // Bind the buffer as the current array buffer. Subsequent glBufferData/glVertexAttribPointer calls refer to this buffer.
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); // GL_STATIC_DRAW hints that data will not change frequenctly
    
    // upload index data — (EBO binding is stored in the VAO) - (to GPU)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(vertexIndices), vertexIndices, GL_STATIC_DRAW);

    // tells OpenGL how to read the vertex data via VAO
    // This line records inside the VAO how attribute location 0 should fetch its data from a VBO.
    glVertexAttribPointer(
        0,                      // attribute location in shader
        3,                      // number of components
        GL_FLOAT,               // data type
        GL_FALSE,               // normalize?
        3 * sizeof(float),      // stride
        (void*)0                // offset
    );

    glEnableVertexAttribArray(0); // Enables the attribute so the GPU will fetch it for each vertex when drawing.
    
    // unbind VBO
    glBindBuffer(GL_ARRAY_BUFFER, 0); 
    glBindVertexArray(0);
    

    // main game loop
    while(!glfwWindowShouldClose(window)){
        /* check game state -> input -> update -> render */
        // process input
        glfwPollEvents();
        
        // render
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        // glUseProgram(shaderProgram);
        shader.Activate();
        glBindVertexArray(VAO);
        
        // glDrawArrays(GL_TRIANGLES, 0, 3); // draw triangle
        glDrawElements(GL_TRIANGLES, 9, GL_UNSIGNED_INT, 0); 

        



        // swap buffers
        glfwSwapBuffers(window);
    }

    // clean up shader
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    // glDeleteProgram(shaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}



