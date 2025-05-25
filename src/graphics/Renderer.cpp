// Renderer.cpp
#include "Renderer.h"

static const char* vertexShaderInCode = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 uMVP;

void main()
{
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)";

static const char* fragmentShaderInCode = R"(
#version 330 core
out vec4 FragColor;

// Uniform con el color
uniform vec3 uObjectColor;

void main()
{
    // alpha=1, color base
    FragColor = vec4(uObjectColor, 1.0);
}
)";

#define NUM_PARTICLES 30000000

struct Particule
{
    EIGEN_ALIGN16 Eigen::Vector4f pos;
    EIGEN_ALIGN16 Eigen::Vector4f base;
    EIGEN_ALIGN16 Eigen::Vector4f color;
};

Renderer::Renderer(int width, int height, const char* title)
    : m_Width(width)
    , m_Height(height)
    , m_FpsSamples(MAX_FPS_SAMPLES, 0.0f)
    , xRotLength(0.0f)
    , yRotLength(0.0f)
    , m_PBFGPU_System()
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
    m_PBFGPU_System.Init();

    m_Camera.SetAspectRatio((float)m_Width / (float)m_Height);
}

Renderer::~Renderer()
{
    Cleanup();
}

bool Renderer::InitGLFW(int width, int height, const char* title)
{
    if (!glfwInit())
    {
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
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
    glfwSetKeyCallback(m_Window, KeyCallback);
    glfwSetMouseButtonCallback(m_Window, MouseButtonCallback);
    glfwSetCursorPosCallback(m_Window, CursorPosCallback);

    // V-Sync
    glfwSwapInterval(1);

    return true;
}

void Renderer::InitOpenGL()
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

void Renderer::InitScene()
{
    // Compilar y linkar shaders de vertices y fragmentos
    bool vertexFragment = m_Shader.CreateShaderProgramFromFiles(
        "..\\src\\graphics\\shaders\\computeVert.vs",
        "..\\src\\graphics\\shaders\\computeFrag.fs"
    );
    /*
    bool vertexFragment = m_Shader.CreateShaderProgramFromFiles(
        "..\\src\\graphics\\shaders\\phong.vs",
        "..\\src\\graphics\\shaders\\phong.fs"
    );
    */
    if (!vertexFragment)
    {
        std::cerr << "Error al crear el shader program desde ficheros\n";
        m_Shader.CreateShaderProgram( vertexShaderInCode, fragmentShaderInCode);
    }

    m_Cube = std::make_unique<Cube>();
    m_Cube->Setup();

    m_Sphere = std::make_unique<Sphere>(0.025f, 16, 32);
    m_Sphere->Setup();

}

void Renderer::Cleanup()
{
    m_ImGuiLayer.Shutdown();

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    glfwDestroyWindow(m_Window);
    glfwTerminate();
}

void Renderer::Run()
{
    while (!glfwWindowShouldClose(m_Window))
    {
        glfwPollEvents();
        m_ImGuiLayer.BeginFrame();

        float fps = 1.0f / ImGui::GetIO().DeltaTime;
        m_AppInfo.fpsSamples[m_AppInfo.sampleIdx] = fps;
        m_AppInfo.sampleIdx = (m_AppInfo.sampleIdx + 1) % m_AppInfo.fpsSamples.size();
        m_ImGuiLayer.ShowInfoPanel(m_AppInfo);
        m_Camera.SetFOV(m_AppInfo.fov);

        glClearColor(0.278f, 0.278f, 0.278f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (buttonState == 1) {
            float newXRot = m_Camera.GetRotation().x() + ((xRotLength - m_Camera.GetRotation().x()) * 0.1f);
            float newYRot = m_Camera.GetRotation().y() + ((yRotLength - m_Camera.GetRotation().y()) * 0.1f);
            m_Camera.SetRotation(Eigen::Vector2f(newXRot, newYRot));
        }
        
        if (enableSimulation)
        {
            m_PBFGPU_System.Step();
        }
            

        //glEnable(GL_BLEND);
        //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        //glDepthMask(GL_FALSE);

        m_Shader.Use();
        Eigen::Matrix4f viewProj = m_Camera.GetProjectionMatrix() *
            m_Camera.GetViewMatrix();
        m_Shader.SetMatrix4("uViewProj", viewProj);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0,
            m_PBFGPU_System.GetParticlesSSBO());

        const GLuint nInst = m_PBFGPU_System.GetNumParticles();

        glBindVertexArray(m_Sphere->GetVAO());
        glDrawElementsInstanced(GL_TRIANGLES,
                                m_Sphere->GetNumIndex(),
                                GL_UNSIGNED_INT,
                                nullptr,
                                nInst);

        //glDepthMask(GL_TRUE);
        //glDisable(GL_BLEND);

        m_ImGuiLayer.EndFrame();
        glfwSwapBuffers(m_Window);
    }
}


void Renderer::FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    // Recuperamos el puntero a la instancia
    Renderer* app = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
    if (app)
    {
        app->m_Width = width;
        app->m_Height = height;
        glViewport(0, 0, width, height);
        app->m_Camera.SetAspectRatio((float)width / (float)height);
    }
}

void Renderer::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // Recuperar el puntero a la instancia de la clase
    Renderer* app = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
    if (app)
    {
        // Delegamos la lógica a un método no estático
        app->HandleKeyCallback(window, key, scancode, action, mods);
    }
}

void Renderer::HandleKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // Solo actuar en el momento en que se presiona la tecla
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        // Un ejemplo: tecla 'W' para incrementar zTrans
        if (key == GLFW_KEY_W) {
            m_Camera.Translate(Eigen::Vector3f( 0.0f, 0.0f, D_TRANSLATION));
        }
        if (key == GLFW_KEY_S) {
            m_Camera.Translate(Eigen::Vector3f( 0.0f, 0.0f,-D_TRANSLATION));
        }
        if (key == GLFW_KEY_A) {
            m_Camera.Translate(Eigen::Vector3f(-D_TRANSLATION, 0.0f, 0.0f));
        }
        if (key == GLFW_KEY_D) {
            m_Camera.Translate(Eigen::Vector3f( D_TRANSLATION, 0.0f, 0.0f));
        }
        if (key == GLFW_KEY_Q) {
            m_Camera.Translate(Eigen::Vector3f( 0.0f,-D_TRANSLATION, 0.0f));
        }
        if (key == GLFW_KEY_E) {
            m_Camera.Translate(Eigen::Vector3f( 0.0f, D_TRANSLATION, 0.0f));
        }
        if (key == GLFW_KEY_SPACE) {
            // Ejemplo: invertir sistema en marcha/parada
            //m_SPHSystem.sys_running = 1 - m_SPHSystem.sys_running;
            enableSimulation = !enableSimulation;
            //std::cout << "Presionaste SPACE (ejemplo): " << m_SPHSystem.sys_running << std::endl;
        }
    }
}

void Renderer::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    Renderer* app = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
    if (app)
    {
        // Delegamos la lógica a un método no estático
        app->HandleMouseButtonCallback(window, button, action, mods);
    }
}

void Renderer::HandleMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS) {
            buttonState = 1;
            //std::cout << "Pulsado" << std::endl;
        }
        else if (action == GLFW_RELEASE) {
            buttonState = 0;
            //std::cout << "Soltado" << std::endl;
        }
    }
}

void Renderer::CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    Renderer* app = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
    if (app)
    {
        app->HandleCursorPosCallback(window, xpos, ypos);
    }
}

void Renderer::HandleCursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    // dx, dy
    double dx = xpos - ox;
    double dy = ypos - oy;

    if (buttonState == 1)
    {
        xRotLength += static_cast<float>(dy) / 5.0f;
        yRotLength += static_cast<float>(dx) / 5.0f;

        //std::cout << "( " << static_cast<float>(dy) / 5.0f << ", " << static_cast<float>(dx) / 5.0f << ")" << std::endl;
    }

    ox = xpos;
    oy = ypos;
}

