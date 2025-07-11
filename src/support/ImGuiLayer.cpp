// ImGuiLayer.cpp
#include "ImGuiLayer.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

void ImGuiLayer::Init(GLFWwindow* window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsClassic();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
}

void ImGuiLayer::BeginFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiLayer::EndFrame()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiLayer::Shutdown()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiLayer::ShowInfoPanel(AppInfo& info)
{
    ImGui::Begin("Info Panel");

    // Slider para modificar el FOV
    ImGui::SliderFloat("FOV", &info.fov, 1.0f, 180.0f);

    // Tomamos el FPS de la �ltima muestra almacenada
    int lastSampleIdx = (info.sampleIdx == 0) ? info.fpsSamples.size() - 1 : info.sampleIdx - 1;
    float currentFps = (lastSampleIdx < info.fpsSamples.size())
        ? info.fpsSamples[lastSampleIdx]
        : 0.0f;

    ImGui::Text("FPS: %.3f", currentFps);

    // Graficamos la l�nea de FPS
    if (!info.fpsSamples.empty())
    {
        ImGui::PlotLines(
            "FPS",
            info.fpsSamples.data(),
            static_cast<int>(info.fpsSamples.size()),
            info.sampleIdx,
            nullptr,
            0.0f,
            info.maxFPS,
            ImVec2(0, 80)
        );
    }

    /* === VRAM === */
    if (!info.vramSamples.empty())
    {
        int lastIdx = (info.vramSampleIdx == 0) ? info.vramSamples.size() - 1
            : info.vramSampleIdx - 1;
        float currentVRAM = info.vramSamples[lastIdx];

        ImGui::Text("VRAM usada: %.1f / %.1f MB",
            currentVRAM, info.maxVRAMPlot);

        ImGui::PlotLines("VRAM (MB)",
            info.vramSamples.data(),
            static_cast<int>(info.vramSamples.size()),
            info.vramSampleIdx,
            nullptr,
            0.0f,
            info.maxVRAMPlot,
            ImVec2(0, 80));
    }


    ImGui::End();
}

