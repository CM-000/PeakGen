cmake_minimum_required(VERSION 3.10)
project(PeakGen)

set(CMAKE_CXX_STANDARD 17)

# Add GLFW (from external folder)
add_subdirectory(external/glfw-3.4)

# Add GLAD (also from external folder)
add_library(glad external/glad/src/glad.c)
target_include_directories(glad PUBLIC external/glad/include)

# Source files
set(SOURCES
    src/main.cpp
    src/terrain.cpp
    src/pathfinding.cpp
    src/shader.cpp
    src/camera.cpp
    src/grid.cpp
)

# Executable
add_executable(PeakGen ${SOURCES})

# Include headers
target_include_directories(PeakGen PRIVATE include)
target_include_directories(PeakGen PRIVATE external/glfw-3.4/include)
target_include_directories(PeakGen PRIVATE external/glm-1.0.2)

# Link GLFW and OpenGL
target_link_libraries(PeakGen PRIVATE glfw glad)
target_link_libraries(PeakGen PRIVATE opengl32)

