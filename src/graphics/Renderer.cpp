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

void Renderer::TestComputeShader()
{
    std::cout << "== TestComputeShader ==\n";

    const int testCount = 10;
    std::vector<PBF_GPU_Particle> testParticles(testCount);

    for (int i = 0; i < testCount; ++i)
    {
        float x = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 2)) - 1.0f;
        float y = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 2)) - 1.0f;
        float z = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 2)) - 1.0f;
        Eigen::Vector4f rndPosition(x, y, z, 1.0f);

        testParticles[i].x = rndPosition;
        testParticles[i].v = Eigen::Vector4f(1, 2, 3, 0);
    }

    // 2. Crear buffers
    GLuint ssboParticles;
    glGenBuffers(1, &ssboParticles);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboParticles);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(PBF_GPU_Particle) * testCount, testParticles.data(), GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboParticles);

    // === PRIMER KERNEL ===
    GLuint velocityProgram = CompileComputeShader("..\\src\\graphics\\compute\\IntegrateAndPredict.comp");
    glUseProgram(velocityProgram);
    glUniform1f(glGetUniformLocation(velocityProgram, "uDeltaTime"), 0.1f);
    glUniform3f(glGetUniformLocation(velocityProgram, "uGravity"), 0.0f, -9.8f, 0.0f);
    glDispatchCompute((testCount + 127) / 128, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // === SEGUNDO KERNEL ===
    // Crear buffers extra
    GLuint ssboCellIndices, ssboIndices;
    glGenBuffers(1, &ssboCellIndices);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboCellIndices);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint) * testCount, nullptr, GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboCellIndices);

    glGenBuffers(1, &ssboIndices);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboIndices);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint) * testCount, nullptr, GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboIndices);
    std::vector<GLuint> initialIndices(testCount);
    for (int i = 0; i < testCount; ++i)
        initialIndices[i] = i;

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboIndices);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint) * testCount, initialIndices.data());

    GLuint cellIndexProgram = CompileComputeShader("..\\src\\graphics\\compute\\AssingCells.comp");
    glUseProgram(cellIndexProgram);
    glUniform3f(glGetUniformLocation(cellIndexProgram, "uGridOrigin"), 0.0f, 1.0f, 0.0f);
    glUniform3i(glGetUniformLocation(cellIndexProgram, "uGridResolution"), 4, 4, 4);
    glUniform1f(glGetUniformLocation(cellIndexProgram, "uCellSize"), 0.1f);
    glDispatchCompute((testCount + 127) / 128, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // === LEER RESULTADOS ===
    PBF_GPU_Particle* outParticles = (PBF_GPU_Particle*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(PBF_GPU_Particle) * testCount, GL_MAP_READ_BIT);
    if (outParticles) {
        for (int i = 0; i < testCount; ++i) {
            std::cout << "Particle " << i << " pred pos: " << outParticles[i].p.transpose() << "\n";
        }
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    }

    // Leer cell indices
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboCellIndices);
    GLuint* cellIdx = (GLuint*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    if (cellIdx) {
        std::cout << "-- Cell Indices --\n";
        for (int i = 0; i < testCount; ++i) {
            std::cout << "Particle " << i << " in cell: " << cellIdx[i] << "\n";
        }
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    }

    // Leer particle indices
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboIndices);
    GLuint* partIdx = (GLuint*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    if (partIdx) {
        std::cout << "-- Particle Indices --\n";
        for (int i = 0; i < testCount; ++i) {
            std::cout << "Index " << i << " = " << partIdx[i] << "\n";
        }
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    }

    // === RADIX SORT TEST ===
    std::cout << "-- Radix Sort Step (1-bit test) --\n";

    // Buffers necesarios para Radix Sort
    GLuint ssboBits, ssboScan, ssboKeysOut, ssboValuesOut, ssboCounter;
    glGenBuffers(1, &ssboBits);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboBits);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint) * testCount, nullptr, GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssboBits); // usar binding 3 por convención nueva

    glGenBuffers(1, &ssboScan);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboScan);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint) * testCount, nullptr, GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssboScan);

    glGenBuffers(1, &ssboKeysOut);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboKeysOut);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint) * testCount, nullptr, GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, ssboKeysOut);

    glGenBuffers(1, &ssboValuesOut);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboValuesOut);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint) * testCount, nullptr, GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, ssboValuesOut);

    glGenBuffers(1, &ssboCounter);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboCounter);
    GLuint zero = 0;
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint), &zero, GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, ssboCounter);

    // === KERNEL 1: ExtractBit ===
    GLuint extractBitProgram = CompileComputeShader("..\\src\\graphics\\compute\\Sort_ExtractBit.comp");
    glUseProgram(extractBitProgram);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboCellIndices); // cell indices as keys
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboBits);
    glUniform1ui(glGetUniformLocation(extractBitProgram, "uNumElements"), testCount);
    glUniform1ui(glGetUniformLocation(extractBitProgram, "uBit"), 0);
    glDispatchCompute((testCount + 127) / 128, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // === KERNEL 2: Scan ===
    GLuint scanProgram = CompileComputeShader("..\\src\\graphics\\compute\\Sort_Scan.comp");
    glUseProgram(scanProgram);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboBits); // binding 1 en shader
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssboScan); // binding 3 en shader
    glUniform1ui(glGetUniformLocation(scanProgram, "uNumElements"), testCount);
    glDispatchCompute((testCount + 127) / 128, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboScan);
    GLuint* scanData = (GLuint*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    if (scanData) {
        std::cout << "-- Scan Result --\n";
        for (int i = 0; i < testCount; ++i) {
            std::cout << "scan[" << i << "] = " << scanData[i] << "\n";
        }
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    }

    // === KERNEL 3: CountOnes ===
    GLuint countProgram = CompileComputeShader("..\\src\\graphics\\compute\\Sort_CountOnes.comp");
    glUseProgram(countProgram);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboBits); // binding 2 en shader
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, ssboCounter); // binding 6 en shader
    glUniform1ui(glGetUniformLocation(countProgram, "uNumElements"), testCount);
    glDispatchCompute((testCount + 127) / 128, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Leer cuántos bits fueron 1 (para calcular total falses)
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboCounter);
    GLuint* countPtr = (GLuint*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    GLuint totalOnes = countPtr ? *countPtr : 0;
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    GLuint totalFalses = testCount - totalOnes;

    // === KERNEL 4: Reorder ===
    GLuint reorderProgram = CompileComputeShader("..\\src\\graphics\\compute\\Sort_Reorder.comp");
    glUseProgram(reorderProgram);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboCellIndices);  // keys in
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboIndices);      // values in
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboBits);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssboScan);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssboKeysOut);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, ssboValuesOut);
    glUniform1ui(glGetUniformLocation(reorderProgram, "uTotalFalses"), totalFalses);
    glUniform1ui(glGetUniformLocation(reorderProgram, "uNumElements"), testCount);
    glDispatchCompute((testCount + 127) / 128, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // === LEER RESULTADOS ===
    std::vector<GLuint> sortedKeys(testCount), sortedIndices(testCount);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboKeysOut);
    GLuint* keysOutPtr = (GLuint*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    if (keysOutPtr) {
        memcpy(sortedKeys.data(), keysOutPtr, sizeof(GLuint) * testCount);
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboValuesOut);
    GLuint* valuesOutPtr = (GLuint*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    if (valuesOutPtr) {
        memcpy(sortedIndices.data(), valuesOutPtr, sizeof(GLuint) * testCount);
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    }

    std::cout << "-- Sorted Cell Indices by Bit 0 --\n";
    for (int i = 0; i < testCount; ++i) {
        std::cout << "Index " << i << " -> Cell: " << sortedKeys[i] << ", Particle Index: " << sortedIndices[i] << "\n";
    }

    // Cleanup extra radix buffers
    glDeleteProgram(extractBitProgram);
    glDeleteProgram(scanProgram);
    glDeleteProgram(countProgram);
    glDeleteProgram(reorderProgram);

    glDeleteBuffers(1, &ssboBits);
    glDeleteBuffers(1, &ssboScan);
    glDeleteBuffers(1, &ssboKeysOut);
    glDeleteBuffers(1, &ssboValuesOut);
    glDeleteBuffers(1, &ssboCounter);

    // === RADIX SORT COMPLETO (32 bits) ===
    GLuint radixKeysIn = ssboCellIndices;
    GLuint radixValuesIn = ssboIndices;

    for (uint bit = 0; bit < 32; ++bit) {
        std::cout << "--- Radix Sort Pass (bit " << bit << ") ---\n";

        // Resetear contador de 1s
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboCounter);
        GLuint zero = 0;
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint), &zero);

        // ExtractBit
        GLuint extract = CompileComputeShader("..\\src\\graphics\\compute\\Sort_ExtractBit.comp");
        glUseProgram(extract);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, radixKeysIn);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboBits);
        glUniform1ui(glGetUniformLocation(extract, "uBit"), bit);
        glDispatchCompute((testCount + 127) / 128, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        glDeleteProgram(extract);

        // Dump bits
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboBits);
        GLuint* bitsPtr = (GLuint*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
        if (bitsPtr) {
            std::cout << "Bits: ";
            for (int i = 0; i < testCount; ++i) std::cout << bitsPtr[i] << " ";
            std::cout << "\n";
            glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        }

        // Scan
        GLuint scan = CompileComputeShader("..\\src\\graphics\\compute\\Sort_Scan.comp");
        glUseProgram(scan);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboBits);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssboScan);
        glDispatchCompute((testCount + 127) / 128, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        glDeleteProgram(scan);

        // Dump scan
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboScan);
        GLuint* scanPtr = (GLuint*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
        if (scanPtr) {
            std::cout << "Scan: ";
            for (int i = 0; i < testCount; ++i) std::cout << scanPtr[i] << " ";
            std::cout << "\n";
            glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        }

        // CountOnes
        GLuint count = CompileComputeShader("..\\src\\graphics\\compute\\Sort_CountOnes.comp");
        glUseProgram(count);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboBits);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, ssboCounter);
        glDispatchCompute((testCount + 127) / 128, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        glDeleteProgram(count);

        GLuint ones = 0;
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboCounter);
        GLuint* countPtr = (GLuint*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
        if (countPtr) ones = *countPtr;
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        GLuint falses = testCount - ones;
        std::cout << "Ones: " << ones << ", Falses: " << falses << "\n";

        // Reorder
        GLuint reorder = CompileComputeShader("..\\src\\graphics\\compute\\Sort_Reorder.comp");
        glUseProgram(reorder);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, radixKeysIn);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, radixValuesIn);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboBits);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssboScan);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssboKeysOut);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, ssboValuesOut);
        glUniform1ui(glGetUniformLocation(reorder, "uTotalFalses"), falses);
        glDispatchCompute((testCount + 127) / 128, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        glDeleteProgram(reorder);

        // Intercambiar buffers para próxima pasada
        std::swap(radixKeysIn, ssboKeysOut);
        std::swap(radixValuesIn, ssboValuesOut);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, radixKeysIn);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, radixValuesIn);
    }

    // === Mostrar resultado final ===
    std::vector<GLuint> finalKeys(testCount), finalIndices(testCount);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, radixKeysIn);
    GLuint* kOut = (GLuint*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    if (kOut) {
        memcpy(finalKeys.data(), kOut, sizeof(GLuint) * testCount);
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, radixValuesIn);
    GLuint* vOut = (GLuint*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    if (vOut) {
        memcpy(finalIndices.data(), vOut, sizeof(GLuint) * testCount);
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    }

    std::cout << "== Final Sorted Result ==\n";
    for (int i = 0; i < testCount; ++i) {
        std::cout << "Index " << i << " -> Cell: " << finalKeys[i] << ", Particle Index: " << finalIndices[i] << "\n";
    }
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

