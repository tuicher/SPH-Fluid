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

// -----------------------------------------------------------------------------
//  Renderer::TestComputeShader()
//  Ejecución de un test GPU end‑to‑end:
//      1) Integrate + Predict positions
//      2) Asignar partículas a celdas
//      3) Radix‑sort de 32 bits totalmente en GPU
// -----------------------------------------------------------------------------
void Renderer::TestComputeShader()
{
    constexpr GLuint kNum = 100;                    // partículas de prueba
    std::cout << "== TestComputeShader ==\n";

    /* ------------------------------------------------------------------
       1.  PARTÍCULAS Y PRIMER KERNEL  (Integrate & Predict)
    ------------------------------------------------------------------ */
    std::vector<PBF_GPU_Particle> particles(kNum);
    for (auto& p : particles)
    {
        Eigen::Vector4f pos = Eigen::Vector4f::Random();   // [-1,1]^3
        pos.w() = 1.0f;
        p.x = pos;
        p.v = Eigen::Vector4f(0.0f, 0.0f, 0.0f, 0.0f);
    }

    GLuint ssboParticles;
    glCreateBuffers(1, &ssboParticles);
    glNamedBufferData(ssboParticles,
        sizeof(PBF_GPU_Particle) * kNum,
        particles.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboParticles);

    GLuint progIntegrate = CompileComputeShader(
        "..\\src\\graphics\\compute\\IntegrateAndPredict.comp");
    glUseProgram(progIntegrate);
    glUniform1f(glGetUniformLocation(progIntegrate, "uDeltaTime"), 0.016f);
    glUniform3f(glGetUniformLocation(progIntegrate, "uGravity"),
        0.0f, -9.81f, 0.0f);
    glDispatchCompute((kNum + 127) / 128, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    /* ------------------------------------------------------------------
       2.  ASIGNAR CÉLULAS
    ------------------------------------------------------------------ */
    GLuint ssboCellIdx, ssboParticleIdx;
    glCreateBuffers(1, &ssboCellIdx);
    glNamedBufferData(ssboCellIdx, sizeof(GLuint) * kNum, nullptr,
        GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboCellIdx);

    glCreateBuffers(1, &ssboParticleIdx);
    std::vector<GLuint> idx(kNum);
    std::iota(idx.begin(), idx.end(), 0);          // 0..N‑1
    glNamedBufferData(ssboParticleIdx, sizeof(GLuint) * kNum,
        idx.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboParticleIdx);

    GLuint progAssign = CompileComputeShader(
        "..\\src\\graphics\\compute\\AssingCells.comp");
    glUseProgram(progAssign);
    glUniform3f(glGetUniformLocation(progAssign, "uGridOrigin"),
        0.0f, 1.0f, 0.0f);
    glUniform3i(glGetUniformLocation(progAssign, "uGridResolution"),
        4, 4, 4);
    glUniform1f(glGetUniformLocation(progAssign, "uCellSize"), 0.1f);
    glDispatchCompute((kNum + 127) / 128, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    /* ------------------------------------------------------------------
       3.  PREPARAMOS BUFFERS AUXILIARES  (¡no se borran hasta el final!)
    ------------------------------------------------------------------ */
    GLuint ssboBits, ssboScan, ssboKeysTmp, ssboValsTmp, ssboCounter;
    glCreateBuffers(1, &ssboBits);
    glNamedBufferData(ssboBits, sizeof(GLuint) * kNum, nullptr,
        GL_DYNAMIC_DRAW);

    glCreateBuffers(1, &ssboScan);
    glNamedBufferData(ssboScan, sizeof(GLuint) * kNum, nullptr,
        GL_DYNAMIC_DRAW);

    glCreateBuffers(1, &ssboKeysTmp);
    glNamedBufferData(ssboKeysTmp, sizeof(GLuint) * kNum, nullptr,
        GL_DYNAMIC_DRAW);

    glCreateBuffers(1, &ssboValsTmp);
    glNamedBufferData(ssboValsTmp, sizeof(GLuint) * kNum, nullptr,
        GL_DYNAMIC_DRAW);

    glCreateBuffers(1, &ssboCounter);
    GLuint zero = 0;
    glNamedBufferData(ssboCounter, sizeof(GLuint), &zero, GL_DYNAMIC_DRAW);

    /* ------------------------------------------------------------------
       4.  COMPILAMOS TODOS LOS SHADERS SÓLO UNA VEZ
    ------------------------------------------------------------------ */
    GLuint progExtract = CompileComputeShader(
        "..\\src\\graphics\\compute\\Sort_ExtractBit.comp");
    GLuint progScan = CompileComputeShader(
        "..\\src\\graphics\\compute\\Sort_Scan.comp");
    GLuint progCount = CompileComputeShader(
        "..\\src\\graphics\\compute\\Sort_CountOnes.comp");
    GLuint progReorder = CompileComputeShader(
        "..\\src\\graphics\\compute\\Sort_Reorder.comp");

    /* ------------------------------------------------------------------
       5.  RADIX SORT GPU ( 32 bits )
    ------------------------------------------------------------------ */
    GLuint keysIn = ssboCellIdx;      // arranque
    GLuint valsIn = ssboParticleIdx;
    GLuint keysOut = ssboKeysTmp;
    GLuint valsOut = ssboValsTmp;

    for (GLuint bit = 0; bit < 32; ++bit)
    {
        /* a) EXTRAER BIT ------------------------------------------------ */
        glUseProgram(progExtract);
        glUniform1ui(glGetUniformLocation(progExtract, "uBit"), bit);
        glUniform1ui(glGetUniformLocation(progExtract, "uNumElements"), kNum);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, keysIn);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboBits);
        glDispatchCompute((kNum + 127) / 128, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        /* b) SCAN ------------------------------------------------------- */
        glUseProgram(progScan);
        glUniform1ui(glGetUniformLocation(progScan, "uNumElements"), kNum);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboBits);   // in
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssboScan);   // out
        glDispatchCompute((kNum + 127) / 128, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        /* c) CONTAR 1s -------------------------------------------------- */
        glNamedBufferSubData(ssboCounter, 0, sizeof(GLuint), &zero);
        glUseProgram(progCount);
        glUniform1ui(glGetUniformLocation(progCount, "uNumElements"), kNum);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboBits);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, ssboCounter);
        glDispatchCompute((kNum + 127) / 128, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        GLuint* onesPtr = (GLuint*)glMapNamedBufferRange(
            ssboCounter, 0, sizeof(GLuint), GL_MAP_READ_BIT);
        GLuint totalOnes = *onesPtr;
        glUnmapNamedBuffer(ssboCounter);
        GLuint totalFalses = kNum - totalOnes;

        /* d) REORDER ---------------------------------------------------- */
        glUseProgram(progReorder);
        glUniform1ui(glGetUniformLocation(progReorder, "uNumElements"), kNum);
        glUniform1ui(glGetUniformLocation(progReorder, "uTotalFalses"),
            totalFalses);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, keysIn);    // keys  in
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, valsIn);    // vals  in
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboBits);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssboScan);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, keysOut);   // keys out
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, valsOut);   // vals out
        glDispatchCompute((kNum + 127) / 128, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        /* e) PING‑PONG -------------------------------------------------- */
        std::swap(keysIn, keysOut);
        std::swap(valsIn, valsOut);
    }

    /* ------------------------------------------------------------------
       6.  RESULTADO
    ------------------------------------------------------------------ */
    std::vector<GLuint> cell(kNum), idxSorted(kNum);

    glGetNamedBufferSubData(keysIn, 0, sizeof(GLuint) * kNum, cell.data());
    glGetNamedBufferSubData(valsIn, 0, sizeof(GLuint) * kNum, idxSorted.data());

    std::cout << "== Final Sorted Result ==\n";
    for (GLuint i = 0; i < kNum; ++i)
        std::cout << "Index " << i << " -> Cell: "
        << cell[i] << ", Particle Index: "
        << idxSorted[i] << '\n';

    std::vector<std::pair<uint, uint>> cpu(kNum);
    for (uint i = 0; i < kNum; ++i) cpu[i] = { cell[i], idxSorted[i] };
    std::is_sorted(cpu.begin(), cpu.end(),
        [](auto a, auto b) { return a.first < b.first; }); // true ⇒ OK

    /* ------------------------------------------------------------------
       7.  LIMPIEZA
    ------------------------------------------------------------------ */
    glDeleteProgram(progIntegrate);
    glDeleteProgram(progAssign);
    glDeleteProgram(progExtract);
    glDeleteProgram(progScan);
    glDeleteProgram(progCount);
    glDeleteProgram(progReorder);

    glDeleteBuffers(1, &ssboParticles);
    glDeleteBuffers(1, &ssboCellIdx);
    glDeleteBuffers(1, &ssboParticleIdx);
    glDeleteBuffers(1, &ssboBits);
    glDeleteBuffers(1, &ssboScan);
    glDeleteBuffers(1, &ssboKeysTmp);
    glDeleteBuffers(1, &ssboValsTmp);
    glDeleteBuffers(1, &ssboCounter);
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

