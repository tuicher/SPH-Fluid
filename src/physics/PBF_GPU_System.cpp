#include "PBF_GPU_System.h"

PBF_GPU_System::PBF_GPU_System()
{
    std::cout << "######## PBF_SYSTEM_GPU ########" << std::endl;
    std::cout << "Sizeof(PBF_GPU_Particle): " << sizeof(PBF_GPU_Particle) << "bytes" << std::endl;
}

PBF_GPU_System::~PBF_GPU_System()
{
    // TODO
}

void PBF_GPU_System::Init()
{
    InitParticles();
    InitSSBOs();
    InitComputeShaders();
}

void PBF_GPU_System::InitParticles()
{
    particles = std::vector<PBF_GPU_Particle>(numParticles);
    Eigen::Vector3f scale{ 0.5f, 2.0f, 0.5f };
    Eigen::Vector3f offset{ 0.0f, 4.0f, 0.0f };

    const double mass = totalMass / numParticles;

    std::cout << "numParticles: " << numParticles << std::endl;
    std::cout << "Mass: " << mass << std::endl;

    GLuint k = 0;
    for (int i = 0; i < numParticles; ++i)
    {
        Eigen::Vector3f p = scale.cwiseProduct(Eigen::Vector3f::Random()) + offset;

        auto& P = particles[i];
        P.x << p, 1.f;
        P.v.setZero();
        P.p = P.x;
        P.color.setZero();
        P.meta << mass, float(k), 0, 0;
    }
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
    glCreateBuffers(1, &ssboSums);
    glNamedBufferData(  ssboSums,
                        sizeof(GLuint) * numParticles,
                        nullptr,
                        GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, ssboSums);
}

void PBF_GPU_System::InitComputeShaders()
{
    // 1) Integrate
    ComputeShader integrateKernel("..\\src\\graphics\\compute\\IntegrateAndPredict.comp");
    integrate = integrateKernel;
    integrate.use();
    integrate.setUniform("uDeltaTime", (float) timeStep);
    integrate.setUniform("uGravity", gravity);

    // 2) Assign Cell
    ComputeShader assignCellKernel("..\\src\\graphics\\compute\\AssignCells.comp");
    assign = assignCellKernel;
    assign.use();
    assign.setUniform("uGridOrigin", Eigen::Vector3f( 0.f, 0.f, 0.f));
    assign.setUniform("uGridResolution", gridRes, gridRes, gridRes);
    assign.setUniform("uCellSize", cellSize);

    // 3) Radix Short
    // a) ExtractBit
    ComputeShader extractBitKernel("..\\src\\graphics\\compute\\Sort_ExtractBit.comp");
    rsExtract = extractBitKernel;
    rsExtract.use();
    rsExtract.setUniform("uNumElements", numParticles);

    // b)
    ComputeShader scanKernel("..\\src\\graphics\\compute\\Sort_BlockScan.comp");
    rsScan = scanKernel;
    rsScan.use();
    rsScan.setUniform("uNumElements", numParticles);

    // c)
    ComputeShader addOffset("..\\src\\graphics\\compute\\Sort_AddOffset.comp");
    rsAddOffset = addOffset;
    rsAddOffset.use();
    rsAddOffset.setUniform("uNumElements", numParticles);

    // d)
    ComputeShader reorder("..\\src\\graphics\\compute\\Sort_Reorder.comp");
    rsReorder = reorder;
    rsReorder.use();
    rsReorder.setUniform("uNumElements", numParticles);
}

void PBF_GPU_System::Step()
{
    /*
    int numGroups = (numParticles + 127) / 128;

    // (1) Integrate & Predict Position
    glUseProgram(programIntegrate);
    glUniform1f(glGetUniformLocation(programIntegrate, "uDeltaTime"), timeStep);
    glUniform3f(glGetUniformLocation(programIntegrate, "uGravity"), 0.0f, -9.81f, 0.0f);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboParticles);
    glDispatchCompute(numGroups, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // (2) Assign Particles to Cells
    glUseProgram(programAssignCells);
    glUniform3f(glGetUniformLocation(programAssignCells, "uGridOrigin"), -2.0f, -2.0f, -2.0f);
    glUniform3i(glGetUniformLocation(programAssignCells, "uGridResolution"), 64, 64, 64);
    glUniform1f(glGetUniformLocation(programAssignCells, "uCellSize"), radius);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboParticles);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboCellIndices);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboParticleIndices);
    glDispatchCompute(numGroups, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // (3) Radix Sort on GPU
    GLuint currentKeys = ssboCellIndices;
    GLuint currentValues = ssboParticleIndices;
    GLuint tempKeys = ssboKeysOut;
    GLuint tempValues = ssboValuesOut;

    for (int bit = 0; bit < 32; ++bit)
    {
        // a) Extract bit
        glUseProgram(programExtractBit);
        glUniform1ui(glGetUniformLocation(programExtractBit, "uBit"), bit);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, currentKeys);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboBitMask);
        glDispatchCompute(numGroups, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        // b) Prefix sum (scan) on bits
        glUseProgram(programScan);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboBitMask);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboScan);
        glDispatchCompute(numGroups, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        // c) Count 1s with atomic
        GLuint zero = 0;
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboBitCounter);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint), &zero);

        glUseProgram(programCountOnes);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboBitMask);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, ssboBitCounter);
        glDispatchCompute(numGroups, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        GLuint totalTrues;
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboBitCounter);
        GLuint* ptr = (GLuint*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint), GL_MAP_READ_BIT);
        totalTrues = ptr[0];
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

        GLuint totalFalses = numParticles - totalTrues;

        // d) Reorder keys & values
        glUseProgram(programReorder);
        glUniform1ui(glGetUniformLocation(programReorder, "uTotalFalses"), totalFalses);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, currentKeys);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, currentValues);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboBitMask);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssboScan);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, tempKeys);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, tempValues);
        glDispatchCompute(numGroups, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        std::swap(currentKeys, tempKeys);
        std::swap(currentValues, tempValues);
    }

    // Después de esto:
    // - currentValues → particleIndices ordenados
    // - currentKeys → cellIndices ordenados

    // (4) TO DO: shader FindCellBounds.comp
    // Generará ssboCellStart y ssboCellEnd desde currentKeys
    */
}

void PBF_GPU_System::Test()
{
    // 1) Integrate
    integrate.use();
    integrate.dispatch(numWorkGroups);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // 2) Hash
    assign.use();
    assign.dispatch(numWorkGroups);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // 3) Radix Short
    for (int bit = 0; bit < 32; ++bit)
    {
        // a) Extract bit
        rsExtract.use();
        rsExtract.setUniform("uBit", bit);
    }
}


