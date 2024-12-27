#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <iostream>
#include <vector>
#include <cmath>

#define MAX_FPS_SAMPLES 256

// Variables globales o estáticas para almacenar el tamaño de la ventana
static int gWidth = 800;
static int gHeight = 600;

// Variable global (o estática) para el FOV en grados
static float gFov = 45.0f;

// Callback para ajustar el viewport y guardar el nuevo tamaño de la ventana
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    gWidth = width;
    gHeight = height;
    glViewport(0, 0, width, height);
}

// Vertex Shader
static const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 uMVP;

void main()
{
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)";

// Fragment Shader
static const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
void main()
{
    FragColor = vec4(1.0, 0.5, 0.2, 1.0);
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

    // Configurar GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Crear ventana
    GLFWwindow* window = glfwCreateWindow(gWidth, gHeight, "SPH-Fluid", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Fallo al crear la ventana GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Desactivar Vsync si quieres (opcional)
    // glfwSwapInterval(0);

    // Inicializar GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Fallo al inicializar GLAD" << std::endl;
        return -1;
    }

    // Callback para cuando se redimensiona la ventana
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Activar depth test para 3D
    glEnable(GL_DEPTH_TEST);

    // Compilar vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    {
        int success;
        char infoLog[512];
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
            std::cerr << "Error al compilar el vertex shader:\n" << infoLog << std::endl;
        }
    }

    // Compilar fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    {
        int success;
        char infoLog[512];
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
            std::cerr << "Error al compilar el fragment shader:\n" << infoLog << std::endl;
        }
    }

    // Crear y enlazar programa
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    {
        int success;
        char infoLog[512];
        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
            std::cerr << "Error al enlazar el programa:\n" << infoLog << std::endl;
        }
    }

    // Eliminar shaders ya que se enlazaron
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Datos para un cubo con índice
    float cubeVertices[] = {
        // Cara frontal (z=+0.5)
        -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        // Cara trasera (z=-0.5)
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f
    };

    unsigned int cubeIndices[] = {
        // Cara frontal
        0, 1, 2,
        2, 3, 0,
        // Cara derecha
        1, 5, 6,
        6, 2, 1,
        // Cara trasera
        5, 4, 7,
        7, 6, 5,
        // Cara izquierda
        4, 0, 3,
        3, 7, 4,
        // Cara superior
        3, 2, 6,
        6, 7, 3,
        // Cara inferior
        4, 5, 1,
        1, 0, 4
    };

    // Crear buffers VAO, VBO, EBO
    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    // Configurar VAO
    glBindVertexArray(VAO);

    // Subir vértices
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    // Subir índices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW);

    // Atributo de posición
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Desvincular
    glBindVertexArray(0);

    // IMGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsClassic();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // Vector para FPS
    std::vector<float> fpsSamples(MAX_FPS_SAMPLES, 0.0f);
    int sampleIndex = 0;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Nuevo frame ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Calcular FPS
        float fps = 1.0f / io.DeltaTime;
        fpsSamples[sampleIndex] = fps;
        sampleIndex = (sampleIndex + 1) % MAX_FPS_SAMPLES;

        // Ventana ImGui: FOV y FPS
        ImGui::Begin("Info Panel");

        // Slider para cambiar FOV en tiempo real
        ImGui::SliderFloat("FOV", &gFov, 1.0f, 180.0f);

        ImGui::Text("FPS: %.3f", fps);
        ImGui::PlotLines("FPS", fpsSamples.data(), MAX_FPS_SAMPLES, sampleIndex, nullptr, 0.0f, 144.0f, ImVec2(0, 80));
        ImGui::End();

        // Limpiar buffers
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Usar programa
        glUseProgram(shaderProgram);

        // 1) Model: rotación continua en Y
        float time = static_cast<float>(glfwGetTime());
        Eigen::Matrix4f model = Eigen::Matrix4f::Identity();
        model *= Eigen::Affine3f(Eigen::AngleAxisf(time, Eigen::Vector3f::UnitY())).matrix();

        // 2) View: cámara a -3.0f en Z
        Eigen::Matrix4f view = Eigen::Matrix4f::Identity();
        view *= Eigen::Affine3f(Eigen::Translation3f(0.0f, 0.0f, -3.0f)).matrix();

        // 3) Projection: recalculada según FOV y aspecto (gWidth/gHeight)
        float aspect = (float)gWidth / (float)gHeight;
        float radFov = gFov * (3.1415926535f / 180.0f);
        float nearPlane = 0.1f;
        float farPlane = 100.0f;

        Eigen::Matrix4f projection = Eigen::Matrix4f::Identity();
        float tanHalfFov = std::tan(radFov / 2.0f);
        projection(0, 0) = 1.0f / (aspect * tanHalfFov);
        projection(1, 1) = 1.0f / tanHalfFov;
        projection(2, 2) = -(farPlane + nearPlane) / (farPlane - nearPlane);
        projection(2, 3) = -(2.0f * farPlane * nearPlane) / (farPlane - nearPlane);
        projection(3, 2) = -1.0f;
        projection(3, 3) = 0.0f;

        // Multiplicación final
        Eigen::Matrix4f mvp = projection * view * model;

        // Cargar la matriz MVP al shader
        GLint mvpLocation = glGetUniformLocation(shaderProgram, "uMVP");
        glUniformMatrix4fv(mvpLocation, 1, GL_FALSE, mvp.data());

        // Dibujar el cubo
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

        // Renderizar ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Swap buffers
        glfwSwapBuffers(window);
    }

    // Limpiar ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // Limpiar OpenGL
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    // Finalizar GLFW
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
