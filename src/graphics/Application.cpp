// Application.cpp
#include "Application.h"
#include <memory>
#include "Camera.h"
#include "Shader.h"
#include "../support/ImGuiLayer.h"
#include "../geometry/Cube.h"
#include "../geometry/Sphere.h"

static const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 uMVP;

void main()
{
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)";

static const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

// Uniform con el color
uniform vec3 uColor;

void main()
{
    // alpha=1, color base
    FragColor = vec4(uColor, 1.0);
}
)";

Application::Application(int width, int height, const char* title)
    : m_Width(width)
    , m_Height(height)
    , m_FpsSamples(MAX_FPS_SAMPLES, 0.0f)
{
    if (!InitGLFW(width, height, title))
    {
        std::cerr << "Error al inicializar GLFW." << std::endl;
        // Manejar error
    }

    m_AppInfo.fpsSamples.resize(256, 0.0f);  // 256 muestras
    m_AppInfo.maxFPS = 144.0f;
    m_AppInfo.fov = 45.0f;

    InitOpenGL();
    InitScene();

    // Ajustar la cámara a la ventana inicial
    m_Camera.SetAspectRatio((float)m_Width / (float)m_Height);
}

Application::~Application()
{
    Cleanup();
}

bool Application::InitGLFW(int width, int height, const char* title)
{
    if (!glfwInit())
    {
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_Window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!m_Window)
    {
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_Window);

    // Callback de cambio de tamaño
    glfwSetWindowUserPointer(m_Window, this);
    glfwSetFramebufferSizeCallback(m_Window, FramebufferSizeCallback);

    return true;
}

void Application::InitOpenGL()
{
    // Cargar funciones OpenGL con GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Error al inicializar GLAD." << std::endl;
        return;
    }

    // glViewport(0, 0, m_Width, m_Height);  // Se ajustará en el callback
    glEnable(GL_DEPTH_TEST);

    // Crear e inicializar ImGui
    m_ImGuiLayer.Init(m_Window);
}

void Application::InitScene()
{
    // Compilar y linkear shader
    //m_Shader.CreateShaderProgram(vertexShaderSource, fragmentShaderSource);

    bool success = m_Shader.CreateShaderProgramFromFiles(
        "C:\\Users\\Javie\\VSCode Projects\\TFM\\SPH-Fluid\\src\\graphics\\shaders\\phong.vs",
        "C:\\Users\\Javie\\VSCode Projects\\TFM\\SPH-Fluid\\src\\graphics\\shaders\\phong.fs"
    );

    if (!success)
    {
        std::cerr << "Error al crear el shader program desde ficheros\n";
        // Maneja el error
    }

    // Crear el cubo y configurarlo
    m_Cube = std::make_unique<Cube>();
    m_Cube->Setup();

    // Crear una esfera y configurarla (radio=1.0, 32 sectores, 32 stacks, por ejemplo)
    m_Sphere = std::make_unique<Sphere>(1.0f, 64, 64);
    m_Sphere->Setup();
}

void Application::Cleanup()
{
    m_ImGuiLayer.Shutdown();

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    glfwDestroyWindow(m_Window);
    glfwTerminate();
}

void Application::Run()
{
    while (!glfwWindowShouldClose(m_Window))
    {
        // 1) Procesar eventos de la ventana
        glfwPollEvents();

        // 2) Comenzar un nuevo frame de ImGui
        m_ImGuiLayer.BeginFrame();

        // 3) Actualizar FPS en m_AppInfo
        float fps = 1.0f / ImGui::GetIO().DeltaTime;
        m_AppInfo.fpsSamples[m_AppInfo.sampleIdx] = fps;
        m_AppInfo.sampleIdx = (m_AppInfo.sampleIdx + 1) % m_AppInfo.fpsSamples.size();

        // 4) Mostrar el panel con los FPS y el FOV
        m_ImGuiLayer.ShowInfoPanel(m_AppInfo);

        // Opcional: si tu cámara requiere el FOV, puedes setearlo aquí
        m_Camera.SetFOV(m_AppInfo.fov);

        // 5) Renderizado de la escena
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Usar tu shader
        m_Shader.Use();

        // Crear la matriz Model (rotación, por ejemplo)
        float time = static_cast<float>(glfwGetTime());
        Eigen::Matrix4f model = Eigen::Matrix4f::Identity();
        model *= Eigen::Affine3f(Eigen::AngleAxisf(time, Eigen::Vector3f::UnitY())).matrix();

        // Tomar View y Projection de la cámara
        Eigen::Matrix4f view = m_Camera.GetViewMatrix();
        Eigen::Matrix4f projection = m_Camera.GetProjectionMatrix();
        Eigen::Matrix4f mvp = projection * view * model;

        m_Shader.SetMatrix4("uMVP", mvp);
        m_Shader.SetVector3f("uColor", m_Cube->GetColor());

        // Dibujar el cubo
        m_Cube->Draw();

        // Dibujar la esfera, movida en X para no superponerse
        Eigen::Matrix4f modelSphere = Eigen::Matrix4f::Identity();
        modelSphere *= Eigen::Affine3f(Eigen::Translation3f(0.0f, 0.0f, -1.5f)).matrix();
        modelSphere *= Eigen::Affine3f(Eigen::AngleAxisf(time, Eigen::Vector3f::UnitZ())).matrix();
        Eigen::Matrix4f mvpSphere = projection * view * modelSphere;
        m_Shader.SetMatrix4("uMVP", mvpSphere);
        m_Shader.SetVector3f("uColor", m_Sphere->GetColor());

        m_Sphere->Draw();

        // 6) Finalizar frame de ImGui
        m_ImGuiLayer.EndFrame();

        // 7) Intercambiar buffers
        glfwSwapBuffers(m_Window);
    }
}

void Application::FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    // Recuperamos el puntero a la instancia
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app)
    {
        app->m_Width = width;
        app->m_Height = height;
        glViewport(0, 0, width, height);
        app->m_Camera.SetAspectRatio((float)width / (float)height);
    }
}
