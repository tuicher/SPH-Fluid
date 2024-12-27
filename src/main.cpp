#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>
#include <vector>

// ----- Eigen -----
#include <Eigen/Core>
#include <Eigen/Geometry>

#define MAX_FPS_SAMPLES 256

// Callback para cambiar el tamaño del viewport al redimensionar la ventana
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// Vertex Shader Source (ahora recibe la matriz MVP)
static const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 uMVP;

void main()
{
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)";

// Fragment Shader Source
static const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

void main()
{
    // Un color naranja similar al original
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

    // Configurar GLFW: Versión de OpenGL y perfil
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Crear la ventana
    GLFWwindow* window = glfwCreateWindow(1280, 720, "SPH-Fluid", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Fallo al crear la ventana GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Hacemos el contexto OpenGL actual
    glfwMakeContextCurrent(window);

    // Desactivamos V-Sync
    glfwSwapInterval(1);

    // Inicializar GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Fallo al inicializar GLAD" << std::endl;
        return -1;
    }

    // Configurar callback de redimensionado
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Activar la prueba de profundidad para 3D
    glEnable(GL_DEPTH_TEST);

    // Compilar Vertex Shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "Error al compilar el shader de vértices:\n" << infoLog << std::endl;
    }

    // Compilar Fragment Shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "Error al compilar el shader de fragmentos:\n" << infoLog << std::endl;
    }

    // Crear y enlazar el programa de shaders
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "Error al enlazar el programa de shaders:\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // -- DEFINICIÓN DE UN CUBO --
    // 8 vértices y 12 triángulos (pero usamos índices para no repetir demasiados datos)
    float cubeVertices[] = {
        //    Positions
        // Cara frontal
        -0.5f, -0.5f,  0.5f,  // V0
         0.5f, -0.5f,  0.5f,  // V1
         0.5f,  0.5f,  0.5f,  // V2
        -0.5f,  0.5f,  0.5f,  // V3

        // Cara trasera
        -0.5f, -0.5f, -0.5f,  // V4
         0.5f, -0.5f, -0.5f,  // V5
         0.5f,  0.5f, -0.5f,  // V6
        -0.5f,  0.5f, -0.5f   // V7
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

    // Crear los buffers
    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    // Vincular VAO
    glBindVertexArray(VAO);

    // VBO: buffer de vértices
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    // EBO: buffer de índices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW);

    // Atributo de posición
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Desvincular (opcional, buena práctica)
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // --- Inicializar IMGUI ---
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsClassic();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // Vector para muestras de FPS
    std::vector<float> fpsSamples(MAX_FPS_SAMPLES, 0.0f);
    int sampleIndex = 0;

    // Bucle de renderizado
    while (!glfwWindowShouldClose(window))
    {
        // Recibir eventos
        glfwPollEvents();

        // Nuevo frame de IMGUI
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        float fps = 1.0f / io.DeltaTime;
        fpsSamples[sampleIndex] = fps;
        sampleIndex = (sampleIndex + 1) % MAX_FPS_SAMPLES;

        // Ventana de IMGUI
        ImGui::Begin("Info Panel");
        ImGui::Text("FPS: %.3f", fps);
        ImGui::PlotLines("FPS", fpsSamples.data(), MAX_FPS_SAMPLES, sampleIndex, nullptr, 0.0f, 100.0f, ImVec2(0, 80));
        ImGui::End();

        // Limpiar color y depth buffer
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Usar nuestro programa
        glUseProgram(shaderProgram);

        // Calcular matrices MVP usando Eigen
        float currentTime = static_cast<float>(glfwGetTime());

        // Model: rotación en Y
        Eigen::Matrix4f model = Eigen::Matrix4f::Identity();
        // Rotación continua en el eje Y
        model *= Eigen::Affine3f(Eigen::AngleAxisf(currentTime, Eigen::Vector3f::UnitY())).matrix();

        // View: mover la cámara un poco hacia atrás en Z
        Eigen::Matrix4f view = Eigen::Matrix4f::Identity();
        // Trasladar la cámara hacia -3 en Z
        view *= Eigen::Affine3f(Eigen::Translation3f(0.0f, 0.0f, -3.0f)).matrix();

        // Projection: perspectiva sencilla
        // fov ~ 45º, aspect ratio 800/600, near plane 0.1, far plane 100
        float fov = 45.0f * (3.1415926535f / 180.0f);
        float aspect = 800.0f / 600.0f;
        float nearPlane = 0.1f;
        float farPlane = 100.0f;

        // Usamos la función de Eigen de perspectiva
        Eigen::Matrix4f projection = Eigen::Matrix4f::Identity();
        float tanHalfFov = std::tan(fov / 2.0f);
        projection(0, 0) = 1.0f / (aspect * tanHalfFov);
        projection(1, 1) = 1.0f / tanHalfFov;
        projection(2, 2) = -(farPlane + nearPlane) / (farPlane - nearPlane);
        projection(2, 3) = -2.0f * farPlane * nearPlane / (farPlane - nearPlane);
        projection(3, 2) = -1.0f;
        projection(3, 3) = 0.0f;

        Eigen::Matrix4f mvp = projection * view * model;

        // Pasar la matriz MVP al shader
        GLint mvpLocation = glGetUniformLocation(shaderProgram, "uMVP");
        glUniformMatrix4fv(mvpLocation, 1, GL_FALSE, mvp.data());

        // Dibujar cubo
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

        // Renderizar IMGUI
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Intercambiar buffers
        glfwSwapBuffers(window);
    }

    // Limpiar IMGUI
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // Borrar recursos
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    // Terminar GLFW
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
