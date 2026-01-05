#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <math.h>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;


const char* vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n" // declare vertex attribute name aPos (vec3). location=0 fixes the attribute location so the application can bind vertex data to location 0 with glVertexAttribPointer(0, ...) 
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n" // Assigns clip-space position for this vertex // gl_Position is a built-in output the pipeline uses for clipping and rasterization
    "}\0";
const char* fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n" // declare the output color variable. Written into framebuffer for each fragment/pixel
    "void main()\n"
    "{\n"
    "   FragColor = vec4(0.8f, 0.3f, 0.00f,1.0f);\n" // Sets the output color to an RGBA value
    "}\n\0";

// Compile shader and print errors if any
auto compile_shader = [](GLuint shader) {
    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint logLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
        std::string log(logLen, ' ');
        glGetShaderInfoLog(shader, logLen, nullptr, &log[0]);
        std::cerr << "Shader compile error:\n" << log << std::endl;
        return false;
    }
    return true;
};

// Link program and print errors if any
auto link_program = [](GLuint prog) {
    GLint success = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success) {
        GLint logLen = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLen);
        std::string log(logLen, ' ');
        glGetProgramInfoLog(prog, logLen, nullptr, &log[0]);
        std::cerr << "Program link error:\n" << log << std::endl;
        return false;
    }
    return true;
};  

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
        -0.8f, -0.5f, 0.0f,
        0.9f, -0.5f, 0.0f,
        0.0f, 0.5f, 0.0f
    };
    
    GLfloat quadVertices[] = {
        -0.5f,  0.5f, 0.0f, // 0: top-left
         0.5f,  0.5f, 0.0f, // 1: top-right
         0.5f, -0.5f, 0.0f, // 2: bottom-right
        -0.5f, -0.5f, 0.0f  // 3: bottom-left
    };
    
    // Indices for two triangles (0,1,2) and (2,3,0)
    unsigned int quadIndices[] = {
        0, 1, 2,
        2, 3, 0
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

    glViewport(0,0, SCREEN_WIDTH, SCREEN_HEIGHT); // Maps normalized device coordinates (NDC, range [-1,1]) to window pixels. // need to change Screen width & height if resize window
    
    // Shader creation, source upload, compilation
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER); // Allocates a new shader object of type vertex and returns its handle.
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr); // Uploads the GLSL source string(s) into the shader object.
    glCompileShader(vertexShader); // Compiles the shader source into GPU-executable code.
    if (!compile_shader(vertexShader)) { 
        std::cerr << "Failed to complied vetex Shader" << std::endl;
        return -1; 
    }
    
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    if (!compile_shader(fragmentShader)) { 
        std::cerr << "Failed to complied fragment Shader" << std::endl;
        return -1; 
    }

    // Program creation, attach, link
    GLuint shaderProgram = glCreateProgram(); // Creates a shader program object that will combine (link) vertex and fragment stages.
    // Attaches compiled shader objects to the program.
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram); // resolves attribute locations, varyings, types and builds a final GPU pipeline object you can use with glUseProgram
    if (!link_program(shaderProgram)) {
        std::cerr << "Failed to link Shader" << std::endl;
        return -1; 
    }   
    
    // clean up shader
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    GLuint VAO, VBO, EBO; // VAO (Vertex Array Object), VBO (Vertex Buffer Object), EBO (Element Buffer Object) or Index buffer
    // Create object names for VAO and VBO.
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO); 
    glGenBuffers(1, &EBO); 

    glBindVertexArray(VAO); // Bind the VAO. A VAO stores vertex attribute state (which VBOs to use, [x,y,z, nx, ny, nz, u, v, ...] ).
                            
    // upload vertex data (to GPU)
    glBindBuffer(GL_ARRAY_BUFFER, VBO); // Bind the buffer as the current array buffer. Subsequent glBufferData/glVertexAttribPointer calls refer to this buffer.
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW); // GL_STATIC_DRAW hints that data will not change frequenctly
    
    // upload index data — (EBO binding is stored in the VAO) - (to GPU)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);

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
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        
        // glDrawArrays(GL_TRIANGLES, 0, 3); // draw triangle
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0); 

        



        // swap buffers
        glfwSwapBuffers(window);
    }

    // clean up shader
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}



