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

    // Ajustar la cámara a la ventana inicial
    m_Camera.SetAspectRatio((float)m_Width / (float)m_Height);

    /// COMPUTE SHADER SEGMENT
    std::vector<Particule> particules(NUM_PARTICLES);

    for (int i = 0; i < particules.size(); i++)
    {
        float x = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 4)) - 2.0f;
        float y = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 4)) - 2.0f;
        float z = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 4)) - 2.0f;

        Eigen::Vector4f base(x, y, z, 1.0f);

        particules[i].base = base;
        particules[i].pos = base;

        // Color aleatorio
        float r = static_cast<float>(rand()) / RAND_MAX;
        float g = static_cast<float>(rand()) / RAND_MAX;
        float b = static_cast<float>(rand()) / RAND_MAX;
        particules[i].color = Eigen::Vector4f(r, g, b, 1.0f);
        //std::cout << i <<": " << base << std::endl;
    }

    std::cout << "Sizeof(Particule): " << sizeof(Particule) << " bytes" << std::endl;


    // Crear el SSBO y subir datos
    glGenBuffers(1, &m_SSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_SSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, particules.size() * sizeof(Particule), particules.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_SSBO);
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
 *--------------------------------------------------------------------
 *  – PBF‑GPU coherente (agua, dx = 1 cm, h = 4 cm).
 *  – Muestra λ y Δp de las 5 primeras partículas.
 *  – Todo en ASCII para evitar artefactos en consolas CP‑1252 / OEM.
 *********************************************************************/
void Renderer::TestComputeShader()
{
    /* ------------------------------------------------------------------ *
     * 0 · CONSTANTES DE ESCENA / SIMULACIÓN
     * ------------------------------------------------------------------ */
    constexpr float   rho0 = 1000.0f;      // kg / m³  (densidad de reposo)
    constexpr float   dx = 0.01f;        // m       (espaciado inicial)
    constexpr float   h = 0.04f;        // m       (radio de suavizado)
    constexpr GLuint  dim = 50;           // 50³ = 125 000 partículas
    constexpr GLuint  N = dim * dim * dim;
    constexpr GLuint  WG = 128;          // local_size_x de *todos* los kernels
    const     GLuint  G = (N + WG - 1) / WG;   // nº de work‑groups
    const     float   mass = rho0 * dx * dx * dx; // masa de cada partícula

    std::cout << "== TestComputeShader ==   N = " << N
        << "   (" << G << " groups of " << WG << ")\n";

    /* ------------------------------------------------------------------ *
     * 1 · GENERAR PARTÍCULAS  (posición, vel =0, etc.)
     * ------------------------------------------------------------------ */
    std::vector<PBF_GPU_Particle> cpuParticles(N);
    {
        GLuint k = 0;
        for (GLuint z = 0; z < dim; ++z)
            for (GLuint y = 0; y < dim; ++y)
                for (GLuint x = 0; x < dim; ++x, ++k)
                {
                    Eigen::Vector3f p{
                        (x - dim * 0.5f) * dx,
                        (y - dim * 0.5f) * dx,
                        (z - dim * 0.5f) * dx };

                    auto& part = cpuParticles[k];
                    part.x << p, 1.0f;          // posición
                    part.v.setZero();          // velocidad
                    part.p = part.x;            // posición “predicha”
                    part.color.setZero();
                    part.meta << mass,
                        static_cast<float>(k), 0.0f, 0.0f;
                }
    }

    GLuint ssboParticles;
    glCreateBuffers(1, &ssboParticles);
    glNamedBufferData(ssboParticles,
        sizeof(PBF_GPU_Particle) * N,
        cpuParticles.data(),
        GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboParticles);   // binding 0

    /* ------------------------------------------------------------------ *
     * 1‑bis · INTEGRATE + PREDICT  (posición pᵢ = xᵢ + vᵢ·dt + ½ g dt²)
     * ------------------------------------------------------------------ */
    {
        GLuint prog = CompileComputeShader(
            "..\\src\\graphics\\compute\\IntegrateAndPredict.comp");

        glUseProgram(prog);
        glUniform1f(glGetUniformLocation(prog, "uDeltaTime"), 0.016f);     // 60 Hz
        glUniform3f(glGetUniformLocation(prog, "uGravity"), 0, -9.81f, 0);
        glDispatchCompute(G, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        glDeleteProgram(prog);
    }

    /* ------------------------------------------------------------------ *
     * 2 · HASHING  (asignar cada partícula a su celda)
     *      SSBO 1  cellKey[N]          (u32)   ←  binding 1
     *      SSBO 2  particleIndex[N]    (u32)   ←  binding 2
     * ------------------------------------------------------------------ */
    GLuint ssboCellKey, ssboParticleIdx;
    glCreateBuffers(1, &ssboCellKey);
    glNamedBufferData(ssboCellKey, sizeof(GLuint) * N, nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboCellKey);     // binding 1

    std::vector<GLuint> idxInit(N);  std::iota(idxInit.begin(), idxInit.end(), 0);
    glCreateBuffers(1, &ssboParticleIdx);
    glNamedBufferData(ssboParticleIdx,
        sizeof(GLuint) * N, idxInit.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboParticleIdx); // binding 2

    {
        GLuint prog = CompileComputeShader(
            "..\\src\\graphics\\compute\\AssignCells.comp");

        // bounding‑box del fluido ligeramente mayor que el cubo de partículas
        constexpr float gridOrigin = -0.22f;   // m
        constexpr GLint  gridRes = 11;       // (0.44 / h) + 1
        glUseProgram(prog);
        glUniform3f(glGetUniformLocation(prog, "uGridOrigin"),
            gridOrigin, gridOrigin, gridOrigin);
        glUniform3i(glGetUniformLocation(prog, "uGridResolution"),
            gridRes, gridRes, gridRes);
        glUniform1f(glGetUniformLocation(prog, "uCellSize"), h);

        glDispatchCompute(G, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        glDeleteProgram(prog);
    }

    /* ------------------------------------------------------------------ *
     * 3 · RADIX‑SORT  (32 bits)  – ordena por cellKey
     *
     *     – keysIn  =  puntero al SSBO que contiene las claves *ordenadas*
     *     – valsIn  =  ”          ”          ”   índices  ”         ”
     * ------------------------------------------------------------------ */
    GLuint ssboBits, ssboScan, ssboKeysTmp, ssboValsTmp,
        ssboCounter, ssboSums, ssboOffsets;
    auto makeBuf = [](GLuint& id, size_t bytes)
        { glCreateBuffers(1, &id);  glNamedBufferData(id, bytes, nullptr, GL_DYNAMIC_DRAW); };

    makeBuf(ssboBits, sizeof(GLuint) * N);
    makeBuf(ssboScan, sizeof(GLuint) * N);
    makeBuf(ssboKeysTmp, sizeof(GLuint) * N);
    makeBuf(ssboValsTmp, sizeof(GLuint) * N);
    makeBuf(ssboCounter, sizeof(GLuint));                 GLuint zero = 0;
    makeBuf(ssboSums, sizeof(GLuint) * G);
    makeBuf(ssboOffsets, sizeof(GLuint) * G);

    glNamedBufferSubData(ssboCounter, 0, 4, &zero);

    auto compileCS = [&](const char* f) { return CompileComputeShader(f); };
    GLuint progExtract = compileCS("..\\src\\graphics\\compute\\Sort_ExtractBit.comp");
    GLuint progBlock = compileCS("..\\src\\graphics\\compute\\Sort_BlockScan.comp");
    GLuint progAddOff = compileCS("..\\src\\graphics\\compute\\Sort_AddOffset.comp");
    GLuint progCount = compileCS("..\\src\\graphics\\compute\\Sort_CountOnes.comp");
    GLuint progReorder = compileCS("..\\src\\graphics\\compute\\Sort_Reorder.comp");

    GLuint keysIn = ssboCellKey, keysOut = ssboKeysTmp;
    GLuint valsIn = ssboParticleIdx, valsOut = ssboValsTmp;

    std::vector<GLuint> cpuSums(G), cpuOffs(G);

    for (GLuint bit = 0; bit < 32; ++bit)
    {
        /* a) bit → ssboBits --------------------------------------------------*/
        glUseProgram(progExtract);
        glUniform1ui(glGetUniformLocation(progExtract, "uBit"), bit);
        glUniform1ui(glGetUniformLocation(progExtract, "uNumElements"), N);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, keysIn);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboBits);
        glDispatchCompute(G, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        /* b‑1) block‑scan ----------------------------------------------------*/
        glUseProgram(progBlock);
        glUniform1ui(glGetUniformLocation(progBlock, "uNumElements"), N);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboBits);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssboScan);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssboSums);
        glDispatchCompute(G, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        /* b‑2) exclusive‑scan CPU de sums[] ---------------------------------*/
        glGetNamedBufferSubData(ssboSums, 0, sizeof(GLuint) * G, cpuSums.data());
        GLuint run = 0;  for (GLuint g = 0; g < G; ++g) { cpuOffs[g] = run; run += cpuSums[g]; }
        glNamedBufferSubData(ssboOffsets, 0, sizeof(GLuint) * G, cpuOffs.data());

        /* b‑3) add offset ----------------------------------------------------*/
        glUseProgram(progAddOff);
        glUniform1ui(glGetUniformLocation(progAddOff, "uNumElements"), N);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssboScan);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, ssboOffsets);
        glDispatchCompute(G, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        /* c) contar unos -----------------------------------------------------*/
        glNamedBufferSubData(ssboCounter, 0, 4, &zero);
        glUseProgram(progCount);
        glUniform1ui(glGetUniformLocation(progCount, "uNumElements"), N);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboBits);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, ssboCounter);
        glDispatchCompute(G, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        GLuint ones;  glGetNamedBufferSubData(ssboCounter, 0, 4, &ones);

        /* d) reordenar claves + índices -------------------------------------*/
        glUseProgram(progReorder);
        glUniform1ui(glGetUniformLocation(progReorder, "uNumElements"), N);
        glUniform1ui(glGetUniformLocation(progReorder, "uTotalFalses"), N - ones);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, keysIn);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, valsIn);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboBits);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssboScan);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, keysOut);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, valsOut);
        glDispatchCompute(G, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        std::swap(keysIn, keysOut);
        std::swap(valsIn, valsOut);
    }
    /*  tras 32 pasadas:  keysIn / valsIn contienen el array ORDENADO  */

    /* ------------------------------------------------------------------ *
     * 4 · FIND‑CELL‑BOUNDS  →  genera   cellStart[C]   y   cellEnd[C]
     * ------------------------------------------------------------------ */
    constexpr GLuint gridRes = 11;                    // mismo que antes
    const     GLuint C = gridRes * gridRes * gridRes;

    GLuint ssboCellStart, ssboCellEnd;
    {
        std::vector<int> initStart(C, INT_MAX), initEnd(C, -1);
        glCreateBuffers(1, &ssboCellStart);
        glNamedBufferData(ssboCellStart, sizeof(int) * C, initStart.data(), GL_DYNAMIC_DRAW);
        glCreateBuffers(1, &ssboCellEnd);
        glNamedBufferData(ssboCellEnd, sizeof(int) * C, initEnd.data(), GL_DYNAMIC_DRAW);
    }

    {
        GLuint prog = CompileComputeShader(
            "..\\src\\graphics\\compute\\FindCellBounds.comp");

        glUseProgram(prog);
        glUniform1ui(glGetUniformLocation(prog, "uNumParticles"), N);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, keysIn);        // keys[]
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboCellStart); // CellStart
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboCellEnd);   // CellEnd

        glDispatchCompute(G, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        glDeleteProgram(prog);
    }

    /* 4‑bis · verificación rápida en CPU -----------------------------------*/
    {
        std::vector<int> startCPU(C), endCPU(C);
        glGetNamedBufferSubData(ssboCellStart, 0, sizeof(int) * C, startCPU.data());
        glGetNamedBufferSubData(ssboCellEnd, 0, sizeof(int) * C, endCPU.data());

        size_t total = 0;
        for (GLuint c = 0; c < C; ++c)
            if (startCPU[c] != INT_MAX) total += endCPU[c] - startCPU[c];

        if (total != N)
            std::cerr << "[FindCellBounds] ERROR  total=" << total << " (esperado " << N << ")\n";
        else
            std::cout << "[FindCellBounds] OK -- vecinos listos\n";
    }

    /* ------------------------------------------------------------------ *
     * 5 · ITERACIÓN PBF   (λ, Δp, aplicar Δp)   –‑ una sola pasada demo
     * ------------------------------------------------------------------ */
    GLuint ssboLambda, ssboDeltaP;
    glCreateBuffers(1, &ssboLambda);
    glNamedBufferData(ssboLambda, sizeof(float) * N, nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssboLambda);       // binding 3

    glCreateBuffers(1, &ssboDeltaP);
    glNamedBufferData(ssboDeltaP, sizeof(Eigen::Vector4f) * N, nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssboDeltaP);       // binding 4

    /*  re‑bind de los SSBO que usan los kernels PBF  -----------------------*/
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboParticles);    // Particles
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, keysIn);           // cellKey ordenado
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, valsIn);           // sorted particleIdx
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, ssboCellStart);    // CellStart
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, ssboCellEnd);      // CellEnd

    /* 5‑a) λ ---------------------------------------------------------------*/
    {
        GLuint prog = CompileComputeShader(
            "..\\src\\graphics\\compute\\ComputeLambda.comp");

        glUseProgram(prog);
        glUniform1ui(glGetUniformLocation(prog, "uNumParticles"), N);
        glUniform1f(glGetUniformLocation(prog, "uRestDensity"), rho0);
        glUniform1f(glGetUniformLocation(prog, "uRadius"), h);
        glUniform1f(glGetUniformLocation(prog, "uEpsilon"), 1e-6f);

        glDispatchCompute(G, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        glDeleteProgram(prog);
    }

    /* 5‑b) Δp --------------------------------------------------------------*/
    {
        GLuint prog = CompileComputeShader(
            "..\\src\\graphics\\compute\\ComputeDeltaP.comp");

        glUseProgram(prog);
        glUniform1ui(glGetUniformLocation(prog, "uNumParticles"), N);
        glUniform1f(glGetUniformLocation(prog, "uRadius"), h);
        glUniform1f(glGetUniformLocation(prog, "uRestDensity"), rho0);
        glUniform1f(glGetUniformLocation(prog, "uSCorrK"), 0.01f);
        glUniform1f(glGetUniformLocation(prog, "uSCorrN"), 4.0f);

        glDispatchCompute(G, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        glDeleteProgram(prog);
    }

    /* 5‑c) aplicar Δp -------------------------------------------------------*/
    {
        GLuint prog = CompileComputeShader(
            "..\\src\\graphics\\compute\\ApplyDeltaP.comp");

        glUseProgram(prog);
        glUniform1ui(glGetUniformLocation(prog, "uNumParticles"), N);

        glDispatchCompute(G, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        glDeleteProgram(prog);
    }

    /* ------------------------------------------------------------------ *
     * 6 · DUMP   (λ y Δp de las 5 primeras partículas) --------------------
     * ------------------------------------------------------------------ */
    constexpr GLuint kDump = 5;
    static_assert(kDump <= N, "kDump > N");

    std::array<float, kDump> lambdaCPU;
    std::array<Eigen::Vector4f, kDump> dPCPU;

    glGetNamedBufferSubData(ssboLambda, 0,
        sizeof(float) * kDump, lambdaCPU.data());
    glGetNamedBufferSubData(ssboDeltaP, 0,
        sizeof(Eigen::Vector4f) * kDump, dPCPU.data());

    std::cout << "\nlambda y dP de las " << kDump << " primeras particulas:\n";
    for (GLuint i = 0; i < kDump; ++i)
        std::cout << "  i=" << i
        << "  lambda=" << lambdaCPU[i]
        << "  dP=(" << dPCPU[i].x()
        << ',' << dPCPU[i].y()
        << ',' << dPCPU[i].z()
        << ")\n";

    /* ------------------------------------------------------------------ *
     * 7 · CLEAN‑UP  (SSBOs temporales y definitivos) -----------------------
     * ------------------------------------------------------------------ */
    GLuint bufsTmp[]{
        ssboBits, ssboScan, ssboKeysTmp, ssboValsTmp,
        ssboCounter, ssboSums, ssboOffsets,
        ssboCellKey, ssboParticleIdx,
        ssboCellStart, ssboCellEnd,
        ssboLambda, ssboDeltaP,
        ssboParticles
    };
    glDeleteBuffers(std::size(bufsTmp), bufsTmp);
}



GLuint Renderer::CompileComputeShader(const std::string& path)
{
    std::string code = LoadFileAsString(path);
    const char* src = code.c_str();

    GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, log);
        std::cerr << "Shader compilation failed:\n" << log << std::endl;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, shader);
    glLinkProgram(program);
    glDeleteShader(shader);
    return program;
}

