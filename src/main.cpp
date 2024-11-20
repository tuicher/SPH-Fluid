#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <Eigen/Dense>

#include <iostream>
#include <vector>

#define MAX_FPS_SAMPLES 256

// Resizing of teh window's callback
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

int main()
{
    // Inicializar GLFW
    if (!glfwInit())
    {
        std::cerr << "Fallo al inicializar GLFW" << std::endl;
        return -1;
    }

    // Configurar GLFW: Versión de OpenGL y perfil
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Crear una ventana de GLFW
    GLFWwindow* window = glfwCreateWindow(800, 600, "SPHfluid", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Fallo al crear la ventana GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Hacer el contexto OpenGL actual
    glfwMakeContextCurrent(window);

    // Inicializar GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Fallo al inicializar GLAD" << std::endl;
        return -1;
    }

    // Configurar el tamaño de la ventana
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Configurar la vista de OpenGL
    glViewport(0, 0, 800, 600);

    // Inicializar ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsDark(); // Usar un estilo oscuro de ImGui

    // Inicializar ImGui para GLFW y OpenGL3
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    Eigen::MatrixXd m(2, 2);
    m(0, 0) = 3;
    m(1, 0) = 2.5;
    m(0, 1) = -1;
    m(1, 1) = m(1, 0) + m(0, 1);
    std::cout << "Here is the matrix m:\n" << m << std::endl;

    // Fps vector
    std::vector<float> fpsSamples(MAX_FPS_SAMPLES, 0.0f);
    int sampleIndex = 0;

    // Bucle de renderizado
    while (!glfwWindowShouldClose(window))
    {
        // Poll Events
        glfwPollEvents();

        // Init new ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        float fps = 1.0f / io.DeltaTime;
        fpsSamples[sampleIndex] = fps;
        sampleIndex = (sampleIndex + 1) % MAX_FPS_SAMPLES;

        // Creating new ImGui window
        ImGui::Begin("Info pannel");

        ImGui::Text("FPS: %.3f", fps);

        ImGui::PlotLines("FPS", fpsSamples.data(), MAX_FPS_SAMPLES, sampleIndex, nullptr, 0.0f, 100.0f, ImVec2(0, 80));

        ImGui::End();

        // Clearing Buffers
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Renderizar contenido de ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Intercambiar buffers
        glfwSwapBuffers(window);
    }

    // Cleaning ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // Terminate GLFW
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
