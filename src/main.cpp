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
#include "Chunk.hpp"

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
        // Bottom (y = 0) - outward normal (0,-1,0)
        {{-0.5f, 0.0f,  0.5f}, { 0.0f,-1.0f, 0.0f}, {1.0f,1.0f,1.0f}, {0.0f,0.0f}}, // 0
        {{-0.5f, 0.0f, -0.5f}, { 0.0f,-1.0f, 0.0f}, {1.0f,1.0f,1.0f}, {1.0f,0.0f}}, // 1
        {{ 0.5f, 0.0f, -0.5f}, { 0.0f,-1.0f, 0.0f}, {1.0f,1.0f,1.0f}, {1.0f,1.0f}}, // 2
        {{ 0.5f, 0.0f,  0.5f}, { 0.0f,-1.0f, 0.0f}, {1.0f,1.0f,1.0f}, {0.0f,1.0f}}, // 3

        // Top (y = 1) - outward normal (0,1,0)
        {{ 0.5f, 1.0f,  0.5f}, { 0.0f, 1.0f, 0.0f}, {1.0f,1.0f,1.0f}, {0.0f,0.0f}}, // 4
        {{ 0.5f, 1.0f, -0.5f}, { 0.0f, 1.0f, 0.0f}, {1.0f,1.0f,1.0f}, {1.0f,0.0f}}, // 5
        {{-0.5f, 1.0f, -0.5f}, { 0.0f, 1.0f, 0.0f}, {1.0f,1.0f,1.0f}, {1.0f,1.0f}}, // 6
        {{-0.5f, 1.0f,  0.5f}, { 0.0f, 1.0f, 0.0f}, {1.0f,1.0f,1.0f}, {0.0f,1.0f}}, // 7

        // Left (x = -0.5) - outward normal (-1,0,0)
        {{-0.5f, 0.0f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f,1.0f,1.0f}, {0.0f,0.0f}}, // 8
        {{-0.5f, 0.0f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f,1.0f,1.0f}, {1.0f,0.0f}}, // 9
        {{-0.5f, 1.0f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f,1.0f,1.0f}, {1.0f,1.0f}}, //10
        {{-0.5f, 1.0f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f,1.0f,1.0f}, {0.0f,1.0f}}, //11

        // Right (x = +0.5) - outward normal (1,0,0)
        {{ 0.5f, 0.0f, -0.5f}, { 1.0f, 0.0f, 0.0f}, {1.0f,1.0f,1.0f}, {0.0f,0.0f}}, //12
        {{ 0.5f, 0.0f,  0.5f}, { 1.0f, 0.0f, 0.0f}, {1.0f,1.0f,1.0f}, {1.0f,0.0f}}, //13
        {{ 0.5f, 1.0f,  0.5f}, { 1.0f, 0.0f, 0.0f}, {1.0f,1.0f,1.0f}, {1.0f,1.0f}}, //14
        {{ 0.5f, 1.0f, -0.5f}, { 1.0f, 0.0f, 0.0f}, {1.0f,1.0f,1.0f}, {0.0f,1.0f}}, //15

        // Front (z = +0.5) - outward normal (0,0,1)
        {{-0.5f, 0.0f,  0.5f}, { 0.0f, 0.0f, 1.0f}, {1.0f,1.0f,1.0f}, {0.0f,0.0f}}, //16
        {{ 0.5f, 0.0f,  0.5f}, { 0.0f, 0.0f, 1.0f}, {1.0f,1.0f,1.0f}, {1.0f,0.0f}}, //17
        {{ 0.5f, 1.0f,  0.5f}, { 0.0f, 0.0f, 1.0f}, {1.0f,1.0f,1.0f}, {1.0f,1.0f}}, //18
        {{-0.5f, 1.0f,  0.5f}, { 0.0f, 0.0f, 1.0f}, {1.0f,1.0f,1.0f}, {0.0f,1.0f}}, //19

        // Back (z = -0.5) - outward normal (0,0,-1)
        {{-0.5f, 0.0f, -0.5f}, { 0.0f, 0.0f,-1.0f}, {1.0f,1.0f,1.0f}, {0.0f,0.0f}}, //20
        {{ 0.5f, 0.0f, -0.5f}, { 0.0f, 0.0f,-1.0f}, {1.0f,1.0f,1.0f}, {1.0f,0.0f}}, //21
        {{ 0.5f, 1.0f, -0.5f}, { 0.0f, 0.0f,-1.0f}, {1.0f,1.0f,1.0f}, {1.0f,1.0f}}, //22
        {{-0.5f, 1.0f, -0.5f}, { 0.0f, 0.0f,-1.0f}, {1.0f,1.0f,1.0f}, {0.0f,1.0f}}, //23
    };

    std::vector<GLuint> vertexIndices = {
        // Bottom
        0,1,2,  2,3,0,
        // Left
        8,9,10, 8,10,11,
        // Right
        12,13,14, 15,12,14,
        // Top
        4,5,6, 4,7,6,
        // Front
        16,17,18, 16,18,19,
        // Back
        20,21,22, 20,22,23
    };

    
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
    std::vector<std::unique_ptr<Texture>> textures;
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
    
    // Enable V-sync
    glfwSwapInterval(1);

    glViewport(0,0, screenWidth, screenHeight); // Maps normalized device coordinates (NDC, range [-1,1]) to window pixels. // need to change Screen width & height if resize window
    
    Shader shader("shaders/basic.vert.glsl", "shaders/basic.frag.glsl");
    Shader lightShader("shaders/light.vert.glsl", "shaders/light.frag.glsl");

    glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    glm::vec3 lightPos = glm::vec3(1.5f, 0.5f, 0.0f);
    glm::mat4 lightModel = glm::mat4(1.0f);
    lightModel = glm::translate(lightModel, lightPos);

    // Texture
    std::unique_ptr popCatTexture = std::make_unique<Texture>("assets/test.png", GL_TEXTURE_2D, GL_TEXTURE0, GL_RGB, GL_UNSIGNED_BYTE);
    popCatTexture->Bind(0);
    shader.AttachTextureUnit(0, "tex0");
    textures.push_back(std::move(popCatTexture));

    float rotation = 0.0f;
    float prevTime = glfwGetTime();
    
    Camera camera(screenWidth, screenHeight, glm::vec3(0.0f, 0.0f, 2.0f));
    // set user pointer and callback
    glfwSetWindowUserPointer(window, &camera);
    glfwSetFramebufferSizeCallback(window, frameBufferCallback);
    
    Mesh cubeMesh(vertices, vertexIndices, std::move(textures), GL_STATIC_DRAW);
    Mesh lightMesh(lightVertices, lightIndices, std::move(emptyTextures), GL_STATIC_DRAW);
    
    Shader chunkShader("shaders/basic.vert.glsl", "shaders/basic.frag.glsl");
    std::vector<Vertex> chunkVertices;
    std::vector<GLuint> chunkIndices;
    std::vector<std::unique_ptr<Texture>> chunkTexture; 
    
    // Texture
    std::unique_ptr chunkGrass = std::make_unique<Texture>("assets/test.png", GL_TEXTURE_2D, GL_TEXTURE0, GL_RGB, GL_UNSIGNED_BYTE);
    chunkGrass->Bind(1);
    chunkShader.AttachTextureUnit(1, "tex0");
    chunkTexture.push_back(std::move(chunkGrass));

    Chunk firstChunk(chunkVertices, chunkIndices, std::move(chunkTexture), GL_STATIC_DRAW);
    firstChunk.GenerateMesh();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

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
        
        float currentTime = glfwGetTime();
        if (currentTime - prevTime >= 1/60){
            rotation += 0.5f;
            prevTime = currentTime;
        }
        
        glm::mat4 model = glm::mat4(1.0f);
        // model = glm::rotate(model, glm::radians(rotation), glm::vec3(1.0f,0.0f,0.0f));
 

 
        shader.Activate();
        shader.SetVec3("lightPos", lightPos);
        shader.SetVec4("lightColor", lightColor);
        shader.SetVec3("camPos", camera.Position);
        int modelLoc = glGetUniformLocation(shader.ID, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        camera.UpdateMatrix(45.0f, 0.1f, 100.0f);
        camera.SetUniformMatrix(shader, "camMatrix");

        cubeMesh.Draw(shader, camera);
        
        lightShader.Activate();
        lightShader.SetMat4("model", lightModel);
        lightShader.SetVec4("lightColor", lightColor);
        camera.SetUniformMatrix(lightShader, "camMatrix");
        lightMesh.Draw(lightShader, camera);

        firstChunk.Render(chunkShader, camera, 1, 1, 0);

        // swap buffers
        glfwSwapBuffers(window);
    }

    // clean up resources
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}




