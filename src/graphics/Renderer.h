// Renderer.h
#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <vector>
#include <iostream>
#include <memory>

#include "Camera.h"
#include "Shader.h"
#include "../support/AppInfo.h"
#include "../support/ImGuiLayer.h"
#include "../geometry/Cube.h"
#include "../geometry/Sphere.h"

// DEFINES
#define D_TRANSLATION 0.2f

class Renderer
{
public:
    Renderer(int width, int height, const char* title);
    ~Renderer();

    void Run();

private:
    bool InitGLFW(int width, int height, const char* title);
    void InitOpenGL();
    void InitScene();
    void Cleanup();

    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    void HandleKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    void HandleMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

    static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    void HandleCursorPosCallback(GLFWwindow* window, double xpos, double ypos);

    GLFWwindow* m_Window = nullptr;
    int m_Width;
    int m_Height;

    double ox, oy;
    float xRotLength, yRotLength;
    int buttonState;

    AppInfo m_AppInfo;         // Info de la app (FOV, FPS, etc.)

    Camera m_Camera;

    static Eigen::Vector3f Translation;

    Shader m_Shader;
    ImGuiLayer m_ImGuiLayer;

    // Buffers
    unsigned int VAO = 0, VBO = 0, EBO = 0;

    // Datos de FPS
    static constexpr int MAX_FPS_SAMPLES = 256;
    std::vector<float> m_FpsSamples;
    int m_SampleIndex = 0;

    // Primitives
    std::unique_ptr<Cube>   m_Cube;
    std::unique_ptr<Sphere> m_Sphere;
};
