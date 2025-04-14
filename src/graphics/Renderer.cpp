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

Renderer::Renderer(int width, int height, const char* title)
    : m_Width(width)
    , m_Height(height)
    , m_FpsSamples(MAX_FPS_SAMPLES, 0.0f)
    , xRotLength(0.0f)
    , yRotLength(0.0f)
    , m_SPHSystem()
    , m_PBFSystem()
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

    // Inicializar el sistema de partículas
    m_SPHSystem.InitSystem();

    // Ajustar la cámara a la ventana inicial
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
    // Compilar y linkear shader
    bool success = m_Shader.CreateShaderProgramFromFiles(
        "..\\src\\graphics\\shaders\\phong.vs",
        "..\\src\\graphics\\shaders\\phong.fs"
    );

    if (!success)
    {
        std::cerr << "Error al crear el shader program desde ficheros\n";
        m_Shader.CreateShaderProgram( vertexShaderInCode, fragmentShaderInCode);
    }

    m_Cube = std::make_unique<Cube>();
    m_Cube->Setup();

    m_Sphere = std::make_unique<Sphere>(0.025f, 3, 3);
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
        // 1) Procesar eventos de la ventana
        glfwPollEvents();

        // 2) Comenzar un nuevo frame de ImGui
        m_ImGuiLayer.BeginFrame();

        if (enableSimulation)
            m_PBFSystem.AnimationStep();

        // 3) Actualizar FPS en m_AppInfo
        float fps = 1.0f / ImGui::GetIO().DeltaTime;
        m_AppInfo.fpsSamples[m_AppInfo.sampleIdx] = fps;
        m_AppInfo.sampleIdx = (m_AppInfo.sampleIdx + 1) % m_AppInfo.fpsSamples.size();

        // 4) Mostrar el panel con los FPS y el FOV
        m_ImGuiLayer.ShowInfoPanel(m_AppInfo);

        // Opcional: si tu cámara requiere el FOV, puedes setearlo aquí
        m_Camera.SetFOV(m_AppInfo.fov);

        m_SPHSystem.Animation();

        // 5) Renderizado de la escena
        glClearColor(0.278f, 0.278f, 0.278f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Camera movement
        if (buttonState == 1)
        {
            float newXRot = m_Camera.GetRotation().x() + ((xRotLength - m_Camera.GetRotation().x()) * 0.1f);
            float newYRot = m_Camera.GetRotation().y() + ((yRotLength - m_Camera.GetRotation().y()) * 0.1f);

            m_Camera.SetRotation(Eigen::Vector2f(newXRot, newYRot));
        }

        // Usar tu shader
        m_Shader.Use();

        Eigen::Matrix4f view = m_Camera.GetViewMatrix();
        Eigen::Matrix4f projection = m_Camera.GetProjectionMatrix();
        m_Shader.SetMatrix4("uProjection", projection);

        // Posición de la luz
        float time = static_cast<float>(glfwGetTime());
        m_Shader.SetVector3f("uLightPos", Eigen::Vector3f(10.0f, 10.0f, 10.0f));
        m_Shader.SetVector3f("uLightColor", Eigen::Vector3f(1.0f, 1.0f, 1.0f));
        // PBF LOOP
        const auto& particles = m_PBFSystem.getParticles();

        for (uint i = 0; i < m_PBFSystem.getNumParticles(); ++i)
        {
            Eigen::Matrix4f model = Eigen::Matrix4f::Identity();

            Vec3 pos = particles[i].x; // ó getParticlePosition(i)
            Eigen::Vector3f position = pos.cast<float>();

            model *= Eigen::Affine3f(Eigen::Translation3f(position)).matrix();

            Eigen::Matrix3f normalMat = model.topLeftCorner<3, 3>().inverse().transpose();
            // Subir matrices al shader
            m_Shader.SetMatrix4("uModelView", view * model);
            m_Shader.SetMatrix3("uNormalMat", normalMat);

            m_Shader.SetVector3f("uObjectColor", particles[i].color);
            m_Sphere->Draw();
        }

        /*
        // SPH LOOP
        // Dibujar todas las esferas en sus posiciones
        for (uint i = 0; i < m_SPHSystem.numParticles; ++i)
        {
            Eigen::Matrix4f model = Eigen::Matrix4f::Identity();

            auto position = m_SPHSystem.mem[i].pos;

            // Aplicar traslación a la posición de la esfera
            model *= Eigen::Affine3f(Eigen::Translation3f(position)).matrix();
            //model *= Eigen::Affine3f(Eigen::AngleAxisf(time, Eigen::Vector3f::UnitY())).matrix();

            Eigen::Matrix3f normalMat = model.topLeftCorner<3, 3>().inverse().transpose();

            // Subir matrices al shader
            m_Shader.SetMatrix4("uModelView", view * model);
            m_Shader.SetMatrix3("uNormalMat", normalMat);

            float p = m_SPHSystem.mem[i].pres;

            // Definir valores de presión mínima y máxima
            float minPressure = 0.0f;
            float maxPressure = 40.0f;

            // Normalizar la presión entre 0 y 1
            float normalizedP = (p - minPressure) / (maxPressure - minPressure);
            normalizedP = std::clamp(normalizedP, 0.0f, 1.0f);

            // Interpolación lineal entre verde y rojo
            Eigen::Vector3f color = (1.0f - normalizedP) * Eigen::Vector3f(1.0f, 1.0f, 1.0f) + normalizedP * Eigen::Vector3f(0.0f, 0.0f, 1.0f);

            // Aplicar el color en el shader
            //m_Shader.SetVector3f("uObjectColor", color);

            m_Shader.SetVector3f("uObjectColor", m_SPHSystem.mem[i].color);

            // Dibujar la esfera
            m_Sphere->Draw();
        }
        */

        /*
        
        int dx = 10;
        int dy = 10;
        int dz = 10;

        for (int i = -dx; i < dx; i++)
        {
            for (int j = -dx; j < dy; j++)
            {
                for (int k = -dx; k < dy; k++)
                {
                    Eigen::Matrix4f modelSphere = Eigen::Matrix4f::Identity();

                    //modelSphere *= Eigen::Affine3f(Eigen::AngleAxisf(time, Eigen::Vector3f::UnitY())).matrix();
                    modelSphere *= Eigen::Affine3f(Eigen::Translation3f(0.2f * i, 0.2f * j, 0.2f * k)).matrix();

                    Eigen::Matrix4f mvpSphere = projection * view * modelSphere;
                    Eigen::Matrix3f normalSphere = modelSphere.topLeftCorner<3, 3>().inverse().transpose();

                    m_Shader.SetMatrix4("uModel", modelSphere);
                    m_Shader.SetMatrix4("uMVP", mvpSphere);
                    m_Shader.SetMatrix3("uNormalMat", normalSphere);

                    m_Shader.SetVector3f("uObjectColor", Eigen::Vector3f((1.0f / dx) * i, (1.0f / dy) * j, (1.0f / dz) * k));

                    m_Sphere->Draw();
                }
            }
        }
        
        // Dibujar la esfera, movida en X para no superponerse
        Eigen::Matrix4f modelSphere = Eigen::Matrix4f::Identity();

        //modelSphere *= Eigen::Affine3f(Eigen::AngleAxisf(time, Eigen::Vector3f::UnitY())).matrix();
        modelSphere *= Eigen::Affine3f(Eigen::Translation3f(0.0f, 0.0f, 0.0f)).matrix();
        modelSphere *= Eigen::Affine3f(Eigen::UniformScaling(15.0f)).matrix();

        Eigen::Matrix4f mvpSphere = projection * view * modelSphere;
        Eigen::Matrix3f normalSphere = modelSphere.topLeftCorner<3, 3>().inverse().transpose();

        m_Shader.SetMatrix4("uModel", modelSphere);
        m_Shader.SetMatrix4("uMVP", mvpSphere);
        m_Shader.SetMatrix3("uNormalMat", normalSphere);
        m_Shader.SetVector3f("uObjectColor", m_Cube->GetColor());

        m_Cube->Draw();
        */
        // 6) Finalizar frame de ImGui
        m_ImGuiLayer.EndFrame();

        // 7) Intercambiar buffers
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
            std::cout << "Presionaste SPACE (ejemplo): " << m_SPHSystem.sys_running << std::endl;
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