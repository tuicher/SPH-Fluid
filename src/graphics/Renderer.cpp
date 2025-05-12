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
    //InitScene();
    m_PBFGPU_System.Init();
    m_PBFGPU_System.Test();
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
    std::string computeCodeStr = LoadFileAsString("..\\src\\graphics\\compute\\simple.comp");
    const char* computeCode = computeCodeStr.c_str();

    GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(shader, 1, &computeCode, nullptr);
    glCompileShader(shader);

    // Verificar errores
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        GLchar infoLog[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
        std::cerr << "Compute Shader error:\n" << infoLog << std::endl;
        return;
    }

    m_ComputeProgram = glCreateProgram(); // ← importante guardarlo así!
    glAttachShader(m_ComputeProgram, shader);
    glLinkProgram(m_ComputeProgram);

    glGetProgramiv(m_ComputeProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        GLchar infoLog[1024];
        glGetProgramInfoLog(m_ComputeProgram, 1024, nullptr, infoLog);
        std::cerr << "Shader linking error:\n" << infoLog << std::endl;
        return;
    }

    glDeleteShader(shader);
    /*
    bool success = m_Shader.CreateShaderProgramFromFiles(
        "..\\src\\graphics\\shaders\\compute_phong.vs",
        "..\\src\\graphics\\shaders\\compute_phong.fs"
    );

    */
    if (!vertexFragment)
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

        // MUY IMPORTANTE: Bindea el SSBO antes de usarlo
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_SSBO);

        // Lanzar compute shader correctamente
        glUseProgram(m_ComputeProgram);
        float currentTime = glfwGetTime();
        GLint uTimeLoc = glGetUniformLocation(m_ComputeProgram, "uTime");
        glUniform1f(uTimeLoc, currentTime);

        glDispatchCompute((NUM_PARTICLES + 999) / 1000, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        // Verificar posiciones actualizadas en CPU (para diagnóstico inmediato)
        /*
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_SSBO);
        Particule* ptr = (Particule*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
        if (ptr) {
            std::cout << "Pos Y primera partícula: " << ptr[0].pos.y() << "\n";
            glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        }
        else {
            std::cerr << "Error mapeando SSBO\n";
        }
        */
        // Dibujo correcto, moderno, y único
        m_Shader.Use();
        Eigen::Matrix4f viewProj = m_Camera.GetProjectionMatrix() * m_Camera.GetViewMatrix();
        m_Shader.SetMatrix4("uViewProj", viewProj);

        glBindVertexArray(m_Sphere->GetVAO());
        glDrawElementsInstanced(GL_TRIANGLES, m_Sphere->GetNumIndex(), GL_UNSIGNED_INT, 0, 25000);

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

/*********************************************************************
 *  Renderer::TestComputeShader()
 *  Versión limpia con ComputeShader + SSBO  (10-May-2025)
 *********************************************************************/
void Renderer::TestComputeShader()
{
    /* ---------------------------------------------------- 0 · Constantes */
    constexpr float  rho0 = 1000.0f;
    constexpr float  dx = 0.01f;
    constexpr float  h = 0.04f;                         // = 4 · dx
    constexpr GLuint dim = 10;                          // 50³ = 125 000
    constexpr GLuint N = dim * dim * dim;
    constexpr GLuint WG = 128;
    const     GLuint G = (N + WG - 1) / WG;
    const     float  mass = rho0 * dx * dx * dx;        // 0.001 kg
    constexpr int    nIter = 8;

    std::cout << "== TestComputeShader ==  N=" << N
        << "   (" << G << " groups × " << WG << " threads)\n";

    /* -------------------------------------------- 1 · Generar partículas */
    std::vector<PBF_GPU_Particle> cpu(N);
    {
        GLuint k = 0;
        for (GLuint z = 0; z < dim; ++z)
            for (GLuint y = 0; y < dim; ++y)
                for (GLuint x = 0; x < dim; ++x, ++k) {
                    Eigen::Vector3f p{ (x - dim * 0.5f) * dx,
                                       (y - dim * 0.5f) * dx,
                                       (z - dim * 0.5f) * dx };
                    auto& P = cpu[k];
                    P.x << p, 1.f;
                    P.v.setZero();
                    P.p = P.x;
                    P.color.setZero();
                    P.meta << mass, float(k), 0, 0;
                }
    }

    /* SSBO[0] : Particles ------------------------------------------------ */
    GLuint ssboParticles;
    glCreateBuffers(1, &ssboParticles);
    glNamedBufferData(ssboParticles, sizeof(PBF_GPU_Particle) * N, cpu.data(),
        GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboParticles);

    /* 1-bis · Integrate + Predict -------------------------------------- */
    {
        ComputeShader integrate("..\\src\\graphics\\compute\\IntegrateAndPredict.comp");
        integrate.use();
        integrate.setUniform("uDeltaTime", 0.016f);
        integrate.setUniform("uGravity", Eigen::Vector3f(0, -9.81f, 0));
        integrate.dispatch(G);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    /* --------------------------- 2 · Hash (cellKey & particleIdx) ----- */
    GLuint ssboCellKey, ssboParticleIdx;
    glCreateBuffers(1, &ssboCellKey);
    glNamedBufferData(ssboCellKey, sizeof(GLuint) * N, nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboCellKey);          // b1

    std::vector<GLuint> initIdx(N); std::iota(initIdx.begin(), initIdx.end(), 0);
    glCreateBuffers(1, &ssboParticleIdx);
    glNamedBufferData(ssboParticleIdx, sizeof(GLuint) * N,
        initIdx.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboParticleIdx);      // b2

    constexpr float  gridOrigin = -0.22f;
    constexpr GLint  gridRes = 11;
    constexpr GLuint C = gridRes * gridRes * gridRes;

    {
        ComputeShader assign("..\\src\\graphics\\compute\\AssignCells.comp");
        assign.use();
        assign.setUniform("uGridOrigin", Eigen::Vector3f(gridOrigin,
            gridOrigin,
            gridOrigin));
        assign.setUniform("uGridResolution", gridRes, gridRes, gridRes);
        assign.setUniform("uCellSize", h);
        assign.dispatch(G);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    /* ---------------------- 3 · Radix-sort (split-scan, 32 bits) ------ */
    GLuint ssboBits, ssboScan, ssboSums, ssboOffsets,
        ssboKeysTmp, ssboValsTmp;
    auto alloc = [&](GLuint& b, size_t sz) {
        glCreateBuffers(1, &b);
        glNamedBufferData(b, sz, nullptr, GL_DYNAMIC_DRAW);
        };
    alloc(ssboBits, sizeof(GLuint) * N);
    alloc(ssboScan, sizeof(GLuint) * N);
    alloc(ssboSums, sizeof(GLuint) * G);
    alloc(ssboOffsets, sizeof(GLuint) * G);
    alloc(ssboKeysTmp, sizeof(GLuint) * N);
    alloc(ssboValsTmp, sizeof(GLuint) * N);

    GLuint keysIn = ssboCellKey, keysOut = ssboKeysTmp;
    GLuint valsIn = ssboParticleIdx, valsOut = ssboValsTmp;

    ComputeShader shExtract("..\\src\\graphics\\compute\\Sort_ExtractBit.comp");
    ComputeShader shScan("..\\src\\graphics\\compute\\Sort_BlockScan.comp");
    ComputeShader shAddOff("..\\src\\graphics\\compute\\Sort_AddOffset.comp");
    ComputeShader shReorder("..\\src\\graphics\\compute\\Sort_Reorder.comp");

    for (GLuint bit = 0; bit < 32; ++bit)
    {
        /* a) extraer bit ------------------------------------------------ */
        shExtract.use();
        shExtract.setUniform("uBit", bit);
        shExtract.setUniform("uNumElements", N);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, keysIn);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboBits);
        shExtract.dispatch(G);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        /* b) scan por bloque ------------------------------------------- */
        shScan.use();
        shScan.setUniform("uNumElements", N);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboBits);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssboScan);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssboSums);
        shScan.dispatch(G);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        /* c) prefix-scan de sums[] en CPU ------------------------------ */
        std::vector<GLuint> sums(G), offs(G);
        glGetNamedBufferSubData(ssboSums, 0, sizeof(GLuint) * G, sums.data());
        GLuint acc = 0;
        for (GLuint g = 0; g < G; ++g) { offs[g] = acc; acc += sums[g]; }
        glNamedBufferSubData(ssboOffsets, 0, sizeof(GLuint) * G, offs.data());
        GLuint numOnes = acc;                          // total bits = 1

        /* d) addOffset -------------------------------------------------- */
        shAddOff.use();
        shAddOff.setUniform("uNumElements", N);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssboScan);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, ssboOffsets);
        shAddOff.dispatch(G);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        /* e) reorden ---------------------------------------------------- */
        shReorder.use();
        shReorder.setUniform("uNumElements", N);
        shReorder.setUniform("uTotalFalses", N - numOnes);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, keysIn);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, valsIn);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboBits);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssboScan);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, keysOut);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, valsOut);
        shReorder.dispatch(G);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        std::swap(keysIn, keysOut);
        std::swap(valsIn, valsOut);
    }

    // Checking Radix Short
    {
        constexpr GLuint kPrint = N;
        std::vector<GLuint> sortedKeys(kPrint), sortedIdx(kPrint);
        glGetNamedBufferSubData(keysIn, 0, sizeof(GLuint) * kPrint, sortedKeys.data());
        glGetNamedBufferSubData(valsIn, 0, sizeof(GLuint) * kPrint, sortedIdx.data());

        std::cout << "\n--- Resultado etapa 3: Radix Sort (primeros " << kPrint << ") ---\n";
        for (GLuint i = 0; i < kPrint; ++i)
        {
            std::cout << " i=" << i << "  cellKey=" << sortedKeys[i]
                << "/" << C << "  particleIdx=" << sortedIdx[i] << '\n';
        }
    }

    /* ----------------------------- 4 · Find-Cell-Bounds -------------- */
    GLuint ssboCellStart, ssboCellEnd;
    std::vector<int> initStart(C, INT_MAX), initEnd(C, -1);
    glCreateBuffers(1, &ssboCellStart);
    glNamedBufferData(ssboCellStart, sizeof(int) * C,
        initStart.data(), GL_DYNAMIC_DRAW);
    glCreateBuffers(1, &ssboCellEnd);
    glNamedBufferData(ssboCellEnd, sizeof(int) * C,
        initEnd.data(), GL_DYNAMIC_DRAW);

    {
        ComputeShader findBounds("..\\src\\graphics\\compute\\FindCellBounds.comp");
        findBounds.use();
        findBounds.setUniform("uNumParticles", N);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, keysIn);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboCellStart);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboCellEnd);
        findBounds.dispatch(G);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    /* 5 · Buffers PBF -------------------------------------------------- */
    GLuint ssboLambda, ssboDeltaP;
    glCreateBuffers(1, &ssboLambda);
    glNamedBufferData(ssboLambda, sizeof(float) * N,
        nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssboLambda);   // b3

    glCreateBuffers(1, &ssboDeltaP);
    glNamedBufferData(ssboDeltaP, sizeof(Eigen::Vector4f) * N,
        nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssboDeltaP);   // b4

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboParticles);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, keysIn);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, valsIn);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, ssboCellStart);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, ssboCellEnd);

    /* 5-x · Shaders PBF (creados una vez) ------------------------------ */
    ComputeShader shLambda("..\\src\\graphics\\compute\\ComputeLambda.comp");
    ComputeShader shDeltaP("..\\src\\graphics\\compute\\ComputeDeltaP.comp");
    ComputeShader shApplyDP("..\\src\\graphics\\compute\\ApplyDeltaP.comp");

    auto setLambdaUniforms = [&]() {
        shLambda.use();
        shLambda.setUniform("uNumParticles", N);
        shLambda.setUniform("uRestDensity", rho0);
        shLambda.setUniform("uRadius", h);
        shLambda.setUniform("uEpsilon", 1e-4f);
        shLambda.setUniform("uGridResolution", gridRes, gridRes, gridRes);
        };
    auto setDeltaUniforms = [&]() {
        shDeltaP.use();
        shDeltaP.setUniform("uNumParticles", N);
        shDeltaP.setUniform("uRadius", h);
        shDeltaP.setUniform("uRestDensity", rho0);
        shDeltaP.setUniform("uSCorrK", 0.001f);
        shDeltaP.setUniform("uSCorrN", 4.0f);
        shDeltaP.setUniform("uGridResolution", gridRes, gridRes, gridRes);
        };
    auto setApplyUniforms = [&]() {
        shApplyDP.use();
        shApplyDP.setUniform("uNumParticles", N);
        };

    /* -------------------- 5 · BUCLE nIter ----------------------------- */
    for (int it = 0; it < nIter; ++it)
    {
        setLambdaUniforms();  shLambda.dispatch(G);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        setDeltaUniforms();   shDeltaP.dispatch(G);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        setApplyUniforms();   shApplyDP.dispatch(G);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    /* -------------------- 6 · Dump depuración (opcional) ------------- */
    /*
    constexpr GLuint kDump = 100;
    std::array<float, kDump> lambda{};
    std::array<Eigen::Vector4f, kDump> deltaP{};
    glGetNamedBufferSubData(ssboLambda, 0,
                            sizeof(float) * kDump, lambda.data());
    glGetNamedBufferSubData(ssboDeltaP, 0,
                            sizeof(Eigen::Vector4f) * kDump, deltaP.data());
    */
    /* -------------------- 7 · Liberar buffers ------------------------ */
    GLuint del[]{
        ssboBits, ssboScan, ssboSums, ssboOffsets,
        ssboKeysTmp, ssboValsTmp, ssboCellKey, ssboParticleIdx,
        ssboCellStart, ssboCellEnd, ssboLambda, ssboDeltaP, ssboParticles
    };
    glDeleteBuffers(std::size(del), del);
}


