// ImGuiLayer.h
#pragma once
#include <GLFW/glfw3.h>
#include "AppInfo.h"

class ImGuiLayer
{
public:
    ImGuiLayer() = default;
    ~ImGuiLayer() = default;

    void Init(GLFWwindow* window);
    void BeginFrame();
    void EndFrame();
    void Shutdown();

    // Panel de información (FOV, FPS, etc.)
    // Usamos un & para poder modificar 'fov' directamente.
    void ShowInfoPanel(AppInfo& info);

private:
    // Podrías agregar configuración adicional si deseas
};

