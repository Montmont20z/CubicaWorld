cmake_minimum_required(VERSION 3.16)
project(CubicaWorld LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

add_executable(cubica
    src/main.cpp
)

target_link_libraries(cubica
    glfw
    OpenGL::GL
)