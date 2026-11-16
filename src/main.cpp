#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <math.h>
#include <stb/stb_image.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <array>
#include <cstdint>
#include <memory>
#include <string>

#include "Logger.hpp"
#include "ChunkManager.hpp"
#include "Shader.hpp"
#include "Texture.hpp"
#include "Camera.hpp"
#include "Mesh.hpp"
#include "RenderSettings.hpp"
#include "PerformanceMetrics.hpp"
#include "DebugOverlay.hpp"

int screenWidth = 1200;
int screenHeight = 1200;

void frameBufferCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    Camera* cam = static_cast<Camera*>(glfwGetWindowUserPointer(window));
    if (cam) cam->SetViewportSize(width, height);
}

static void errorCallback(int error, const char* description) {
    LOG_ERROR("GLFW error " + std::to_string(error) + ": " + description);
}

#ifdef DEBUG_MODE
void APIENTRY OpenGLDebugCallback(
    GLenum source, GLenum type, GLuint id, GLenum severity,
    GLsizei length, const GLchar* message, const void* userParam) {
    (void)source; (void)type; (void)userParam;
    LogLevel level;
    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:   level = LogLevel::Error; break;
        case GL_DEBUG_SEVERITY_MEDIUM: level = LogLevel::Warning; break;
        case GL_DEBUG_SEVERITY_LOW:    level = LogLevel::Debug; break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: return;
        default: level = LogLevel::Warning; break;
    }
    Logger::Log(level, "OpenGL [" + std::to_string(id) + "] " + std::string(message, length));
}
#endif

int main() {
    Logger::Init("logs/CubicaWorld.log");
    LOG_INFO("Application starting");

    glfwSetErrorCallback(errorCallback);
    if (!glfwInit()) {
        LOG_ERROR("Failed to initialize GLFW");
        glfwTerminate();
        Logger::Shutdown();
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // do not use deprecated functions (eg glBegin, glEnd)
#ifdef DEBUG_MODE
    LOG_DEBUG("Debug Mode Enabled");
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "Cubica World", nullptr, nullptr);
    if (!window) {
        LOG_ERROR("Failed to create GLFW window");
        glfwTerminate();
        Logger::Shutdown();
        return -1;
    }
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        LOG_ERROR("Failed to initialize GLAD");
        glfwDestroyWindow(window);
        glfwTerminate();
        Logger::Shutdown();
        return -1;
    }

    glfwSwapInterval(0); // Uncapped performance testing; use 1 for normal V-Sync gameplay.

    glViewport(0,0, screenWidth, screenHeight); // Maps normalized device coordinates (NDC, range [-1,1]) to window pixels. // need to change Screen width & height if resize window
    {
    Shader chunkShader("shaders/chunk.vert.glsl", "shaders/chunk.frag.glsl");
    ChunkManager chunkManager(VIEW_DISTANCE, IS_SINGLE_THREADED);

    const glm::vec4 lightColor(1.0f);
    const glm::vec3 lightPos(1.5f, 0.5f, 0.0f);

    auto textureAtlas = std::make_unique<Texture>(
        "assets/DefaultPack.png", GL_TEXTURE_2D, GL_TEXTURE0, GL_UNSIGNED_BYTE);
    textureAtlas->Bind(0);
    chunkShader.Activate();
    glUniform1i(glGetUniformLocation(chunkShader.ID, "atlasTiles"), 16);
    chunkShader.AttachTextureUnit(0, "tex0");

    Camera camera(screenWidth, screenHeight, glm::vec3(0.0f, 68.0f, 0.0f));
    glfwSetWindowUserPointer(window, &camera);
    glfwSetFramebufferSizeCallback(window, frameBufferCallback);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LESS);

#ifdef DEBUG_MODE
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(OpenGLDebugCallback, nullptr);
#endif

    const bool overlayInitialized = DebugOverlay::Initialize(window);
    if (!overlayInitialized) {
        LOG_ERROR("Failed to initialize Dear ImGui debug overlay; continuing without the overlay");
    }

    // Start with the overlay visible. F3 toggles it during runtime.
    bool showDebugOverlay = overlayInitialized;
    glfwSetInputMode(window, GLFW_CURSOR,
        showDebugOverlay ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);

    // Asynchronous GPU timer-query ring. Never wait for a result in the same frame.
    constexpr std::size_t GPU_QUERY_COUNT = 5;
    std::array<GLuint, GPU_QUERY_COUNT> gpuQueries{};
    std::array<bool, GPU_QUERY_COUNT> gpuQueryPending{};
    std::array<std::uint64_t, GPU_QUERY_COUNT> gpuQueryFrame{};
    std::uint64_t frameCounter = 0;
    std::uint64_t latestGpuFrame = 0;
    glGenQueries(static_cast<GLsizei>(GPU_QUERY_COUNT), gpuQueries.data());
    std::size_t nextGpuQuery = 0;
    double latestGpuChunkMs = 0.0;

    double previousFrameStart = glfwGetTime();
    bool f3WasDown = false;

    while (!glfwWindowShouldClose(window)) {
        const double frameStart = glfwGetTime();
        const double deltaSeconds = frameStart - previousFrameStart;
        previousFrameStart = frameStart;
        PerformanceMetrics::Instance().RecordFrameTime(deltaSeconds * 1000.0);

        glfwPollEvents();

        const bool f3Down = glfwGetKey(window, GLFW_KEY_F3) == GLFW_PRESS;
        if (f3Down && !f3WasDown && overlayInitialized) {
            showDebugOverlay = !showDebugOverlay;
            glfwSetInputMode(window, GLFW_CURSOR,
                showDebugOverlay ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
        }
        f3WasDown = f3Down;

        camera.ProcessInput(window);

        // Resolve completed queries without stalling the CPU.
        for (std::size_t i = 0; i < GPU_QUERY_COUNT; ++i) {
            if (!gpuQueryPending[i]) continue;
            GLint available = GL_FALSE;
            glGetQueryObjectiv(gpuQueries[i], GL_QUERY_RESULT_AVAILABLE, &available);
            if (available == GL_TRUE) {
                GLuint64 elapsedNanoseconds = 0;
                glGetQueryObjectui64v(gpuQueries[i], GL_QUERY_RESULT, &elapsedNanoseconds);
                if (gpuQueryFrame[i] >= latestGpuFrame) {
                    latestGpuChunkMs = static_cast<double>(elapsedNanoseconds) / 1'000'000.0;
                    latestGpuFrame = gpuQueryFrame[i];
                    PerformanceMetrics::Instance().SetGpuChunkTime(latestGpuChunkMs);
                }
                gpuQueryPending[i] = false;
            }
        }
        glClearColor(0.53f, 0.82f, 0.98f, 1.0f); // sky blue
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        camera.UpdateMatrix(45.0f, 0.1f, 500.0f);

        chunkShader.Activate();
        chunkShader.SetVec3("lightPos", lightPos);
        chunkShader.SetVec4("lightColor", lightColor);

        // Select a free query. If all are pending, skip querying this frame rather than block.
        int activeQueryIndex = -1;
        for (std::size_t offset = 0; offset < GPU_QUERY_COUNT; ++offset) {
            const std::size_t candidate = (nextGpuQuery + offset) % GPU_QUERY_COUNT;
            if (!gpuQueryPending[candidate]) {
                activeQueryIndex = static_cast<int>(candidate);
                break;
            }
        }

        if (activeQueryIndex >= 0) {
            glBeginQuery(GL_TIME_ELAPSED, gpuQueries[static_cast<std::size_t>(activeQueryIndex)]);
        }
        chunkManager.Update(camera.Position, chunkShader, camera);
        if (activeQueryIndex >= 0) {
            glEndQuery(GL_TIME_ELAPSED);
            gpuQueryPending[static_cast<std::size_t>(activeQueryIndex)] = true;
            gpuQueryFrame[static_cast<std::size_t>(activeQueryIndex)] = frameCounter;
            nextGpuQuery = (static_cast<std::size_t>(activeQueryIndex) + 1) % GPU_QUERY_COUNT;
        }

        if (showDebugOverlay && overlayInitialized) {
            DebugOverlay::BeginFrame();
            DebugOverlay::Draw(chunkManager, PerformanceMetrics::Instance().GetSnapshot());
            DebugOverlay::Render();
        }

        glfwSwapBuffers(window);
        ++frameCounter;
    }

    glDeleteQueries(static_cast<GLsizei>(GPU_QUERY_COUNT), gpuQueries.data());
    if (overlayInitialized) DebugOverlay::Shutdown();
    } // Destroy ChunkManager, Shader and Texture while the OpenGL context is still alive.

    LOG_INFO("Application shutting down");
    glfwDestroyWindow(window);
    glfwTerminate();
    Logger::Shutdown();
    return 0;
}