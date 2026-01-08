cmake_minimum_required(VERSION 3.16)
project(CubicaWorld LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

# GLAD
add_library(glad src/glad.c)
target_include_directories(glad PUBLIC include)

add_executable(cubica
    src/main.cpp
    src/Shader.cpp
    src/VAO.cpp
    src/VBO.cpp
    src/EBO.cpp
    src/stb.cpp
)

target_link_libraries(cubica
    PRIVATE
    glad
    glfw
    OpenGL::GL
)


# ---------- Copy shaders & assets directory to the runtime (binary) folder ----------
add_custom_command(TARGET cubica POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:cubica>/shaders
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_SOURCE_DIR}/shaders
          $<TARGET_FILE_DIR:cubica>/shader
  COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:cubica>/assets
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_SOURCE_DIR}/assets
          $<TARGET_FILE_DIR:cubica>/assets
)
# --------------------------------------------------------------------------