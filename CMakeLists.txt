cmake_minimum_required(VERSION 3.10)

include(FetchContent)

#  SPH-Fluid
project(SPHfluid VERSION 1.0 LANGUAGES CXX)

# SPHfluid dir
set(SPHfluid_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# C++ 17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# GLFW - 3.4
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
)
FetchContent_MakeAvailable(glfw)

# ImGui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.1
)
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
  FetchContent_Populate(imgui)
  add_library(imgui STATIC 
    ${imgui_SOURCE_DIR}/imgui.cpp 
    ${imgui_SOURCE_DIR}/imgui_draw.cpp 
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp 
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp 
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
  target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends ${glfw_SOURCE_DIR}/include)
  target_link_libraries(imgui PRIVATE glfw)
endif()

# Glad - thirdparty
add_library(glad STATIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glad/src/glad.c
)
target_include_directories(glad PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glad/include
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glad/include/KHR
)

# Eigen
FetchContent_Declare(
    eigen
    GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
    GIT_TAG        3.4.0
)

FetchContent_GetProperties(eigen)
if(NOT eigen_POPULATED)
  FetchContent_Populate(eigen)
endif()

# OpenGL
find_package(OpenGL REQUIRED)

# Define el ejecutable de tu proyecto, los archivos fuente y de cabecera
file(GLOB PROJECT_SOURCES src/*.cpp)
file(GLOB PROJECT_HEADERS src/*.h)
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES} ${PROJECT_HEADERS})

target_link_libraries(${PROJECT_NAME} PRIVATE 
    OpenGL::GL 
    glfw
    imgui
    glad
)

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glad/include
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glad/include/KHR
    ${eigen_SOURCE_DIR}
)
