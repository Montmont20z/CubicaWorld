cmake_minimum_required(VERSION 3.16)
project(CubicaWorld LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

# GLAD
add_library(glad STATIC src/third_party/glad.c)
target_include_directories(glad PUBLIC ${CMAKE_SOURCE_DIR}/include)

# STB
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${CMAKE_SOURCE_DIR}/include)

add_library(glm INTERFACE)
target_include_directories(glm INTERFACE ${CMAKE_SOURCE_DIR}/include) 

add_executable(cubica
    src/third_party/stb.cpp
    src/main.cpp
    src/Shader.cpp
    src/VAO.cpp
    src/EBO.cpp
    src/Texture.cpp
    src/Camera.cpp
    # src/Mesh.cpp
    src/Chunk.cpp
    src/ChunkManager.cpp
    src/ChunkMesh.cpp
    src/TerrainGenerator.cpp
    src/Logger.cpp
)

target_link_libraries(cubica
    PRIVATE
    glad
    glfw
    OpenGL::GL
    stb
    glm
)

# Enable debug features only in Debug configuration
target_compile_definitions(cubica PRIVATE
    $<$<CONFIG:Debug>:DEBUG_MODE>
)

# ---------- Copy shaders & assets directory to the runtime (binary) folder ----------
add_custom_command(TARGET cubica POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:cubica>/shaders
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_SOURCE_DIR}/shaders
          $<TARGET_FILE_DIR:cubica>/shaders
  COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:cubica>/assets
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_SOURCE_DIR}/assets
          $<TARGET_FILE_DIR:cubica>/assets
)
# --------------------------------------------------------------------------
# AddressSanitizer for Debug builds (GCC / Clang)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")

    target_compile_options(cubica PRIVATE
        $<$<CONFIG:Debug>:-fsanitize=address>
        $<$<CONFIG:Debug>:-fno-omit-frame-pointer>
    )

    target_link_options(cubica PRIVATE
        $<$<CONFIG:Debug>:-fsanitize=address>
    )

endif()