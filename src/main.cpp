#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>
#include <vector>

#define MAX_FPS_SAMPLES 256

// Resizing of the window's callback
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// Vertex Shader Source
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

void main()
{
    gl_Position = vec4(aPos, 1.0);
}
)";

// Fragment Shader Source
const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

void main()
{
    FragColor = vec4(1.0, 0.5, 0.2, 1.0); // Orange color
}
)";

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
    GLFWwindow* window = glfwCreateWindow(800, 600, "Hello Triangle", nullptr, nullptr);
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

    // Crear y compilar los shaders
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    // Verificar errores en el shader de vértices
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "Error al compilar el shader de vértices:\n" << infoLog << std::endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    // Verificar errores en el shader de fragmentos
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "Error al compilar el shader de fragmentos:\n" << infoLog << std::endl;
    }

    // Crear programa de shader y enlazar shaders
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Verificar errores en el programa de shaders
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "Error al enlazar el programa de shaders:\n" << infoLog << std::endl;
    }

    // Eliminar shaders intermedios
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Definir los datos del triángulo
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f
    };

    // Crear VBO y VAO
    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    // Configurar VAO y VBO
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0); // Desvincular VBO
    glBindVertexArray(0); // Desvincular VAO

    // Inicializar ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsClassic();

    // Inicializar ImGui para GLFW y OpenGL3
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

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
        ImGui::Begin("Info Panel");

        ImGui::Text("FPS: %.3f", fps);
        ImGui::PlotLines("FPS", fpsSamples.data(), MAX_FPS_SAMPLES, sampleIndex, nullptr, 0.0f, 100.0f, ImVec2(0, 80));

        ImGui::End();

        // Renderizar triángulo
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

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

    // Limpiar recursos de OpenGL
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    // Terminar GLFW
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
