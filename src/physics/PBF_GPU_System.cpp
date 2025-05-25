#include "PBF_GPU_System.h"

PBF_GPU_System::PBF_GPU_System()
{
    std::cout << "######## PBF_SYSTEM_GPU ########" << std::endl;
    std::cout << "Sizeof(PBF_GPU_Particle): " << sizeof(PBF_GPU_Particle) << "bytes" << std::endl;
}

PBF_GPU_System::~PBF_GPU_System()
{
    auto del = [](GLuint& id)
        {
            if (id != 0)
                glDeleteBuffers(1, &id);
            id = 0;
        };

    del(ssboParticles);
    del(ssboCellKey);
    del(ssboParticleIdx);
    del(ssboBits);
    del(ssboScan);
    del(ssboSums);
    del(ssboOffsets);
    del(ssboKeysTmp);
    del(ssboValsTmp);
    del(ssboCellStart);
    del(ssboCellEnd);
    del(ssboLambda);
    del(ssboDeltaP);
    del(ssboDensity);
    del(ssboDeltaV);
}

void PBF_GPU_System::Init()
{
    InitParticles();
    SetParticlesColors();
    InitSSBOs();
    InitComputeShaders();
    InitSimulation();
}

void PBF_GPU_System::InitParticles()
{
    particles = std::vector<PBF_GPU_Particle>(numParticles);
    Eigen::Vector3f scale{ 0.5f, 2.0f, 0.5f };
    Eigen::Vector3f offset{ 0.0f, 6.0f, 0.0f };

    std::cout << "numParticles: " << numParticles << std::endl;
    std::cout << "Mass: " << massPerParticle << std::endl;

    GLuint k = 0;
     
    for (int i = 0; i < numParticles; ++i)
    {
        Eigen::Vector3f p = scale.cwiseProduct(Eigen::Vector3f::Random()) + offset;
        
        // Genera posiciones dentro de una esfera de radio centrada en (0, 5, 0)
        //Eigen::Vector3f p = Eigen::Vector3f::Random().normalized();
        //p *= 0.05f;
        //p = p + Eigen::Vector3f(0.0f, 5.0f, 0.0f);

        auto& P = particles[i];
        P.x << p, 1.f;

        P.v.setZero();
        P.p = P.x;
        P.color = Eigen::Vector4f( 0.016f, 0.0f, 0.639f, 1.0f);
        //P.color = Eigen::Vector4f::Random();
        P.meta << massPerParticle, float(k), 0, 0;
    }
}

void PBF_GPU_System::SetParticlesColors()
{
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();

    for (const auto& p : particles) {
        float y = p.x.y();
        minY = std::min(minY, y);
        maxY = std::max(maxY, y);
    }
    if (maxY == minY) maxY += 1.0f;     // evitar división 0

    for (auto& p : particles) {

        // Normalizar (0-1) invertido: 0=arriba, 1=abajo
        float t = (maxY - p.x.y()) / (maxY - minY);

        // HSV → RGB   (Hue 0-300º)
        float hue = t * 300.0f;
        float s = 1.0f, v = 1.0f;

        float c = v * s;
        float hprime = hue / 60.0f;
        float x = c * (1.0f - std::fabs(std::fmod(hprime, 2.0f) - 1.0f));
        float m = v - c;

        float r, g, b;
        switch (static_cast<int>(hprime) % 6) {
        case 0: r = c; g = x; b = 0; break;
        case 1: r = x; g = c; b = 0; break;
        case 2: r = 0; g = c; b = x; break;
        case 3: r = 0; g = x; b = c; break;
        case 4: r = x; g = 0; b = c; break;
        case 5: r = c; g = 0; b = x; break;
        default: r = g = b = 0;       break;
        }

        p.color << std::clamp(r + m, 0.0f, 1.0f),
            std::clamp(g + m, 0.0f, 1.0f),
            std::clamp(b + m, 0.0f, 1.0f),
            1.0f;
    }
}

void PBF_GPU_System::InitSimulation()
{
    for (int i = 0; i < numRelaxSteps; i++)
        Step(timeStep / numRelaxSteps);
    

    resetVelocity.use();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboParticles);

    resetVelocity.dispatch(numWorkGroups);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

}

void PBF_GPU_System::UpdateGrid()
{
    // 1. Leer posiciones de la GPU (o mantén una copia CPU si ya la tienes)
    std::vector<PBF_GPU_Particle> cpuBuffer(numParticles);
    glGetNamedBufferSubData(ssboParticles, 0,
        sizeof(PBF_GPU_Particle) * numParticles,
        cpuBuffer.data());

    Eigen::Vector3f minPos(FLT_MAX, FLT_MAX, FLT_MAX);
    Eigen::Vector3f maxPos(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    for (const auto& p : cpuBuffer)
    {
        minPos = minPos.cwiseMin(p.x.head<3>());
        maxPos = maxPos.cwiseMax(p.x.head<3>());
    }

    // 2. Margen = h para que las vecinas quepan
    float pad = cellSize;
    Eigen::Vector3f origin = minPos - Eigen::Vector3f::Ones() * pad;
    Eigen::Vector3f extent = (maxPos - minPos) + 2.0f * Eigen::Vector3f::Ones() * pad;

    // 3. Nº celdas (ceil) + 1
    Eigen::Array3i res = ((extent / cellSize).array()).ceil().cast<int>() + 1;
    gridRes = res;
    GLuint totCells = gridRes.prod();

    // 4. Asegura que los buffers tienen tamaño suficiente
    if (totCells > currentTotCells)
    {
        ResizeCellBuffers(totCells);
        currentTotCells = totCells;
    }

    gridOrigin = origin;

    // 5. Sube uniforms a TODOS los kernels implicados
    auto pushGrid = [&](ComputeShader& cs)
        {
            cs.use();
            cs.setUniform("uGridOrigin", gridOrigin);
            cs.setUniform("uGridResolution", gridRes);
            cs.setUniform("uCellSize", cellSize);
        };

    pushGrid(assign);
    pushGrid(computeLambda);
    pushGrid(computeDeltaP);
    pushGrid(computeDensity);
    pushGrid(applyViscosity);
}

void PBF_GPU_System::InitSSBOs()
{
    // [0] - SSBO Particles
    glCreateBuffers(1, &ssboParticles);
    glNamedBufferData(  ssboParticles, 
                        sizeof(PBF_GPU_Particle) * numParticles, 
                        particles.data(),
                        GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboParticles);

    // [1] - cellKey
    glCreateBuffers(1, &ssboCellKey);
    glNamedBufferData(  ssboCellKey,
                        sizeof(GLuint) * numParticles,
                        nullptr,
                        GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboCellKey);

    // [2] - Particle Idx
    std::vector<GLuint> initIdx(numParticles);
    std::iota(initIdx.begin(), initIdx.end(), 0);

    glCreateBuffers(1, &ssboParticleIdx);
    glNamedBufferData(  ssboParticleIdx,
                        sizeof(GLuint) * numParticles,
                        initIdx.data(),
                        GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboParticleIdx);

    // ### - Radix-sort (split-scan, 32 bits)
    // [3] - Bits
    glCreateBuffers(1, &ssboBits);
    glNamedBufferData(  ssboBits,
                        sizeof(GLuint) * numParticles,
                        nullptr,
                        GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssboBits);

    // [4] - Scan
    glCreateBuffers(1, &ssboScan);
    glNamedBufferData(  ssboScan,
                        sizeof(GLuint) * numParticles,
                        nullptr,
                        GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssboScan);

    // [5] - Sums
    glCreateBuffers(1, &ssboSums);
    glNamedBufferData(  ssboSums,
                        sizeof(GLuint) * numWorkGroups,
                        nullptr,
                        GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, ssboSums);

    // [6] - Offsets
    glCreateBuffers(1, &ssboOffsets);
    glNamedBufferData(  ssboOffsets,
                        sizeof(GLuint) * numWorkGroups,
                        nullptr,
                        GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, ssboOffsets);

    // [7] - KeysTmp
    glCreateBuffers(1, &ssboKeysTmp);
    glNamedBufferData(ssboKeysTmp,
                        sizeof(GLuint) * numParticles,
                        nullptr,
                        GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, ssboKeysTmp);

    // [8] - ValsTmp
    glCreateBuffers(1, &ssboValsTmp);
    glNamedBufferData(ssboValsTmp,
                        sizeof(GLuint) * numParticles,
                        nullptr,
                        GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, ssboValsTmp);

    // [9] - CellStart
    std::vector<int> initStart(totCells, INT_MAX);
    glCreateBuffers(1, &ssboCellStart);
    glNamedBufferData(  ssboCellStart, 
                        sizeof(int) * totCells,
                        initStart.data(),
                        GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, ssboCellStart);

    // [10] - CellEnd
    std::vector<int> initEnd(totCells, -1);
    glCreateBuffers(1, &ssboCellEnd);
    glNamedBufferData(  ssboCellEnd,
                        sizeof(int) * totCells,
                        initEnd.data(),
                        GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, ssboCellEnd);

    // [11] - Lambdas
    glCreateBuffers(1, &ssboLambda);
    glNamedBufferData(  ssboLambda,
                        sizeof(float) * numParticles,
                        nullptr,
                        GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, ssboLambda);

    // [12] - DeltaPs
    glCreateBuffers(1, &ssboDeltaP);
    glNamedBufferData(  ssboDeltaP,
                        sizeof(Eigen::Vector4f) * numParticles,
                        nullptr,
                        GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 12, ssboDeltaP);

    // [13] – Density
    glCreateBuffers(1, &ssboDensity);
    glNamedBufferData(  ssboDensity,
                        sizeof(float)* numParticles,
                        nullptr,
                        GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 13, ssboDensity);

    // [14] – DeltaV
    glCreateBuffers(1, &ssboDeltaV);
    glNamedBufferData(  ssboDeltaV,
                        sizeof(Eigen::Vector4f)* numParticles,
                        nullptr, 
                        GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 14, ssboDeltaV);
}

void PBF_GPU_System::InitComputeShaders()
{
    // 1) Integrate
    integrate = ComputeShader("..\\src\\graphics\\compute\\IntegrateAndPredict.comp");
    integrate.use();
    integrate.setUniform("uDeltaTime", (float)timeStep);
    integrate.setUniform("uGravity", gravity);

    // 2) Assign Cell
    assign = ComputeShader("..\\src\\graphics\\compute\\AssignCells.comp");
    assign.use();
    assign.setUniform("uGridOrigin", gridOrigin);
    assign.setUniform("uGridResolution", gridRes);
    assign.setUniform("uCellSize", cellSize);

    // 3) Radix Short
    // a) ExtractBit
    rsExtract = ComputeShader("..\\src\\graphics\\compute\\Sort_ExtractBit.comp");
    rsExtract.use();
    rsExtract.setUniform("uNumElements", numParticles);

    // b)

    rsScan = ComputeShader("..\\src\\graphics\\compute\\Sort_BlockScan.comp");
    rsScan.use();
    rsScan.setUniform("uNumElements", numParticles);

    // c)
    rsAddOffset = ComputeShader("..\\src\\graphics\\compute\\Sort_AddOffset.comp");
    rsAddOffset.use();
    rsAddOffset.setUniform("uNumElements", numParticles);

    // d)
    rsReorder = ComputeShader("..\\src\\graphics\\compute\\Sort_Reorder.comp");
    rsReorder.use();
    rsReorder.setUniform("uNumElements", numParticles);

    // 4) Find-Cell-Bounds
    findBounds = ComputeShader("..\\src\\graphics\\compute\\FindCellBounds.comp");
    findBounds.use();
    findBounds.setUniform("uNumElements", numParticles);

    // 5) PBF
    // 5.a - Compute Lambdas
    computeLambda = ComputeShader("..\\src\\graphics\\compute\\ComputeLambda.comp");
    computeLambda.use();
    computeLambda.setUniform("uNumParticles", numParticles);
    computeLambda.setUniform("uRestDensity", (float)restDensity);
    computeLambda.setUniform("uRadius", (float)radius);
    computeLambda.setUniform("uEpsilon", (float)epsilon);
    computeLambda.setUniform("uGridResolution", gridRes);

    // 5.b - Compute DeltaPs
    computeDeltaP = ComputeShader("..\\src\\graphics\\compute\\ComputeDeltaP.comp");
    computeDeltaP.use();
    computeDeltaP.setUniform("uNumParticles", numParticles);
    computeDeltaP.setUniform("uRadius", (float)radius);
    computeDeltaP.setUniform("uRestDensity", (float)restDensity);
    computeDeltaP.setUniform("uSCorrK", (float)massPerParticle * 1e-4f);
    computeDeltaP.setUniform("uSCorrN", 4.0f);
    computeDeltaP.setUniform("uGridResolution", gridRes);

    // 5.c - Apply DeltaPs
    applyDeltaP = ComputeShader("..\\src\\graphics\\compute\\ApplyDeltaP.comp");
    applyDeltaP.use();
    applyDeltaP.setUniform("uNumParticles", numParticles);

    // 6) Update Velocity
    updateVelocity = ComputeShader("..\\src\\graphics\\compute\\UpdateVelocity.comp");
    updateVelocity.use();
    updateVelocity.setUniform("uDeltaTime", (float)timeStep);
    updateVelocity.setUniform("uDamping", (float)damping);

    // 7-a  Density for XSPH
    computeDensity = ComputeShader("..\\src\\graphics\\compute\\ComputeDensity.comp");
    computeDensity.use();
    computeDensity.setUniform("uNumParticles", numParticles);
    computeDensity.setUniform("uMass", (float)massPerParticle);
    computeDensity.setUniform("uRadius", (float)radius);
    computeDensity.setUniform("uGridOrigin", gridOrigin);
    computeDensity.setUniform("uGridResolution", gridRes);
    computeDensity.setUniform("uCellSize", cellSize);
    computeDensity.setUniform("INT_MAX", initStart);

    // 7-b  Apply viscosity
    applyViscosity = ComputeShader("..\\src\\graphics\\compute\\ApplyViscosity.comp");
    applyViscosity.use();
    applyViscosity.setUniform("uNumParticles", numParticles);
    applyViscosity.setUniform("uMass", (float)massPerParticle);
    applyViscosity.setUniform("uRadius", (float)radius);
    applyViscosity.setUniform("uViscosity", (float)viscosity);
    applyViscosity.setUniform("uGridOrigin", gridOrigin);
    applyViscosity.setUniform("uGridResolution", gridRes);
    applyViscosity.setUniform("uCellSize", cellSize);
    applyViscosity.setUniform("INT_MAX", initStart);

    // 8) ?

    // 9) Resolve Collisions
    resolveCollisions = ComputeShader("..\\src\\graphics\\compute\\ResolveCollisions.comp");
    resolveCollisions.use();
    resolveCollisions.setUniform("uNumParticles", numParticles);
    resolveCollisions.setUniform("uMinBound", MinBound);
    resolveCollisions.setUniform("uMaxBound", MaxBound);
    resolveCollisions.setUniform("uRestitution", 0.95f);

    resetVelocity = ComputeShader("..\\src\\graphics\\compute\\ResetVelocities.comp");
    resetVelocity.use();
    resetVelocity.setUniform("uNumParticles", numParticles);
}

void PBF_GPU_System::Step()
{
    for (int i = 0; i < numSubSteps; i++)
    {
        Step(subTimeStep);
    }
}

void PBF_GPU_System::Step(float dt)
{
    UpdateGrid();

    // 1) Integrate
    integrate.use();
    integrate.setUniform("uDeltaTime", dt);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboParticles);
    integrate.dispatch(numWorkGroups);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // 2) Hash
    assign.use();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboParticles);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboCellKey);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboParticleIdx);
    assign.dispatch(numWorkGroups);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // 3) Radix Short
    for (GLuint bit = 0; bit < 32; ++bit)
    {
        // a) Extract bit
        rsExtract.use();
        rsExtract.setUniform("uBit", bit);
        rsExtract.setUniform("uNumElements", numParticles);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboCellKey);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssboBits);
        rsExtract.dispatch(numWorkGroups);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        // b) BlockScan
        rsScan.use();
        rsScan.setUniform("uNumElements", numParticles);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssboBits);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssboScan);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, ssboSums);
        rsScan.dispatch(numWorkGroups);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        // c) prefixScan - in CPU
        std::vector<GLuint> sums(numWorkGroups), offs(numWorkGroups);
        glGetNamedBufferSubData(    ssboSums, 
                                    0, 
                                    sizeof(GLuint) * numWorkGroups,
                                    sums.data());
        GLuint numOnes = 0;
        for (GLuint g = 0; g < numWorkGroups; ++g)
        { 
            offs[g] = numOnes;
            numOnes += sums[g];
                
        }
        glNamedBufferSubData(   ssboOffsets, 
                                0,
                                sizeof(GLuint) * numWorkGroups,
                                offs.data());;
        
        // d) addOffset
        rsAddOffset.use();
        rsAddOffset.setUniform("uNumElements", numParticles);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssboScan);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, ssboOffsets);
        rsAddOffset.dispatch(numWorkGroups);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
#ifdef DEBUG
        if (verbose)
        {
            int n = 32;
            std::vector<GLuint> bits(n), scan(n);
            glGetNamedBufferSubData(ssboBits, 0, n * sizeof(GLuint), bits.data());
            glGetNamedBufferSubData(ssboScan, 0, n * sizeof(GLuint), scan.data());
            std::cout << "Bit [" << bit << "]\n";
            std::cout << "bits : \t"; for (auto b : bits)  std::cout << b << ' '; std::cout << '\n';
            std::cout << "scan : \t"; for (auto s : scan)  std::cout << s << ' '; std::cout << '\n';
        }
        else
        {
            const GLuint Ncheck = 8 * 1024;
            std::vector<GLuint> bitsCPU(Ncheck), scanGPU(Ncheck);

            glGetNamedBufferSubData(ssboBits, 0, Ncheck * sizeof(GLuint), bitsCPU.data());
            glGetNamedBufferSubData(ssboScan, 0, Ncheck * sizeof(GLuint), scanGPU.data());

            bool ok = true;
            GLuint prefix = 0;
            for (GLuint i = 0; i < Ncheck && ok; ++i)
            {
                if (scanGPU[i] != prefix)              ok = false;
                prefix += bitsCPU[i];
            }
            if (ok && prefix != scanGPU.back() + bitsCPU.back()) ok = false;   // último valor

            std::cout << "[CHECK bit " << bit << "] scan " << (ok ? "OK\n"
                : "ERROR: mismatch in prefix-scan\n");
        }
#endif // DEBUG

        // e) Reorder
        rsReorder.use();
        rsReorder.setUniform("uNumElements", numParticles);
        rsReorder.setUniform("uTotalFalses", numParticles - numOnes);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboCellKey);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboParticleIdx);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssboBits);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssboScan);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, ssboKeysTmp);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, ssboValsTmp);
        rsReorder.dispatch(numWorkGroups);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        std::swap(ssboCellKey, ssboKeysTmp);
        std::swap(ssboParticleIdx, ssboValsTmp);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboCellKey);      // Keys actuales
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboParticleIdx);  // Vals actuales
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, ssboKeysTmp);      // Keys temporales
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, ssboValsTmp);      // Vals temporales
    }
#ifdef DEBUG
    // Checking Radix Short
    if (verbose)
    {
        const GLuint kPrint = numParticles;
        std::vector<GLuint> sortedKeys(kPrint), sortedIdx(kPrint);
        glGetNamedBufferSubData(ssboCellKey, 0, sizeof(GLuint) * kPrint, sortedKeys.data());
        glGetNamedBufferSubData(ssboParticleIdx, 0, sizeof(GLuint) * kPrint, sortedIdx.data());
        const GLuint C = gridRes * gridRes * gridRes;

        std::cout << "\n--- Resultado etapa 3: Radix Sort (primeros " << kPrint << ") ---\n";
        for (GLuint i = 0; i < kPrint; ++i)
        {
            std::cout << " i=" << i << "  cellKey=" << sortedKeys[i]
                << "/" << C << "  particleIdx=" << sortedIdx[i] << '\n';
        }
    }
    else
    {
        std::vector<GLuint> k(numParticles);
        glGetNamedBufferSubData(ssboCellKey, 0,
            numParticles * sizeof(GLuint), k.data());

        bool sorted = std::is_sorted(k.begin(), k.end());
        bool withinRange = *std::max_element(k.begin(), k.end()) < gridRes * gridRes * gridRes;

        std::cout << "[CHECK radix sort] "
            << (sorted ? "sorted" : "NOT sorted") << " | "
            << (withinRange ? "keys OK\n" : "keys fuera de rango\n");
    }
#endif // DEBUG

    // 4) Find-Cell-Bounds
    findBounds.use();
    glClearNamedBufferData( ssboCellStart, 
                            GL_R32I,
                            GL_RED_INTEGER,
                            GL_INT,
                            &initStart);
    glClearNamedBufferData( ssboCellEnd,    
                            GL_R32I,
                            GL_RED_INTEGER,
                            GL_INT,
                            &initEnd);
    findBounds.dispatch(numWorkGroups);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT);
#ifdef DEBUG
    std::vector<int> start(totCells), end(totCells);
    glGetNamedBufferSubData(ssboCellStart, 0, totCells * sizeof(int), start.data());
    glGetNamedBufferSubData(ssboCellEnd, 0, totCells * sizeof(int), end.data());
    if (verbose)
    {
        std::cout << "\n--- Cell bounds ---\n";
        for (GLuint c = 0; c < totCells; ++c)
            if (start[c] != INT_MAX)
                std::cout << "cell " << c << " : [" << start[c] << ", " << end[c] << ")\n";
    }
    else
    {
        bool allOk = true;
        int prevEnd = 0, total = 0;

        for (GLuint c = 0; c < totCells; ++c)
        {
            int s = start[c], e = end[c];
            if (s == INT_MAX && e == -1) continue;   // celda vacía ― nada que comprobar
            // (a) índices consistentes
            if (!(0 <= s && s < e && e <= (int)numParticles)) allOk = false;
            // (b) adyacentes a la celda anterior
            if (s != prevEnd) allOk = false;
            prevEnd = e;
            total += e - s;
        }
        allOk &= (total == (int)numParticles);

        std::cout << "[CHECK cell bounds] "
            << (allOk ? "OK (covers 100 %)"
                : "ERROR: huecos/solapes en las celdas")
            << '\n';
    }
#endif // DEBUG 

    // 5) PBF Steps
    
    for (int it = 0; it < numIter; ++it)
    {
        // 5.a Compute Lambdas
        computeLambda.use();
    
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboParticles);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboCellKey);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboParticleIdx);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, ssboCellStart);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, ssboCellEnd);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, ssboLambda);

        computeLambda.dispatch(numWorkGroups);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    
#ifdef DEBUG
    constexpr GLuint kPrint = 16;
    std::array<float, kPrint> lambda{};
    glGetNamedBufferSubData(ssboLambda, 0,
        sizeof(float) * kPrint, lambda.data());
    std::cout << "lambda[0.." << kPrint - 1 << "]:\n";
    for (auto v : lambda) std::cout << v << '\n';
#endif // DEBUG

        // 5.b Compute DeltaPs
        computeDeltaP.use();

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboParticles);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboCellKey);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboParticleIdx);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, ssboCellStart);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, ssboCellEnd);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, ssboLambda);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 12, ssboDeltaP);

        computeDeltaP.dispatch(numWorkGroups);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

#ifdef DEBUG  
    std::array<Eigen::Vector4f, kPrint> dp{};
    glGetNamedBufferSubData(ssboDeltaP,
        0,
        sizeof(Eigen::Vector4f)* kPrint,
        dp.data());
    std::cout << "deltaP[0.." << kPrint - 1 << "].xyz :\n";
    for (auto& v : dp)
        std::cout << '(' << v.x() << ',' << v.y() << ',' << v.z() << ") \n";
    std::cout << '\n';
#endif // DEBUG

        // 5.c Apply DeltaPs
        applyDeltaP.use();
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboParticles);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 12, ssboDeltaP);

#ifdef DEBUG  
    std::vector<PBF_GPU_Particle> before(numParticles);
    std::vector<Eigen::Vector4f>  deltaP(numParticles);

    glGetNamedBufferSubData(ssboParticles,
        0,
        sizeof(PBF_GPU_Particle) * numParticles,
        before.data());

    glGetNamedBufferSubData(ssboDeltaP,
        0,
        sizeof(Eigen::Vector4f) * numParticles,
        deltaP.data());
#endif // DEBUG

        applyDeltaP.dispatch(numWorkGroups);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }
#ifdef DEBUG 
    // Copy Post Values
    std::vector<PBF_GPU_Particle> after(numParticles);
    glGetNamedBufferSubData(ssboParticles,
        0,
        sizeof(PBF_GPU_Particle)* numParticles,
        after.data());

    // Compare
    double maxErr = 0.0, rms = 0.0;
    const int Nshow = 16;
    int shown = 0;

    for (GLuint i = 0; i < numParticles; ++i)
    {
        Eigen::Vector3f expected = before[i].p.head<3>() + deltaP[i].head<3>();
        Eigen::Vector3f got = after[i].p.head<3>();
        double err = (got - expected).norm();

        maxErr = std::max(maxErr, err);
        rms += err * err;

        if (err > 1e-6 && shown < Nshow)
        {
            std::cout << "i=" << i
                << "  exp(" << expected.transpose() << ")"
                << "  gpu(" << got.transpose() << ")"
                << "  |err|=" << err << '\n';
            ++shown;
        }
    }
    rms = std::sqrt(rms / numParticles);

    std::cout << "DeltaP CHECK  →  max-error = " << maxErr
        << "   RMS-error = " << rms << '\n';
#endif // DEBUG

    // 6 Update Velocity
    updateVelocity.use();
    updateVelocity.setUniform("uDeltaTime", dt);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboParticles);
    updateVelocity.dispatch(numWorkGroups);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // 7-a) Compute densities for XSPH
    computeDensity.use();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboParticles);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboCellKey);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboParticleIdx);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, ssboCellStart);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, ssboCellEnd);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 13, ssboDensity);
    computeDensity.dispatch(numWorkGroups);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // 7-b) Apply viscosity
    applyViscosity.use();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboParticles);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboCellKey);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboParticleIdx);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, ssboCellStart);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, ssboCellEnd);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 13, ssboDensity);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 14, ssboDeltaV);
    applyViscosity.dispatch(numWorkGroups);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // 8 ?

    // 9) Collisions
    resolveCollisions.use();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboParticles);
    resolveCollisions.dispatch(numWorkGroups);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void PBF_GPU_System::Test(int n)
{
    for (int i = 0; i < n; ++i)
    {
        std::cout << "Test: " << i + 1 << std::endl;
        Step();
    }
}

void PBF_GPU_System::ResizeCellBuffers(GLuint newTotCells)
{
    // Libera los SSBO antiguos
    glDeleteBuffers(1, &ssboCellStart);
    glDeleteBuffers(1, &ssboCellEnd);

    // Crea otros nuevos con el tamaño solicitado
    std::vector<int> initStart(newTotCells, INT_MAX);
    std::vector<int> initEnd(newTotCells, -1);

    glCreateBuffers(1, &ssboCellStart);
    glNamedBufferData(ssboCellStart,
        sizeof(int) * newTotCells,
        initStart.data(),
        GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, ssboCellStart);

    glCreateBuffers(1, &ssboCellEnd);
    glNamedBufferData(ssboCellEnd,
        sizeof(int) * newTotCells,
        initEnd.data(),
        GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, ssboCellEnd);
}

