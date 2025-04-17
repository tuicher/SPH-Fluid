#include "PBF_GPU_System.h"

// SSBO necesarios
/*
Ejemplo de definición de SSBO en GLSL (compute shader)
layout(std430, binding = 0) buffer Positions { vec4 positions[]; };         // posición actual
layout(std430, binding = 1) buffer PredPositions { vec4 predictedPos[]; };  // posición predicha
layout(std430, binding = 2) buffer Velocities { vec4 velocities[]; };       // velocidad
layout(std430, binding = 3) buffer Lambdas { float lambdas[]; };            // lambda por partícula
layout(std430, binding = 4) buffer CellIndex { uint particleCellIndices[]; };
layout(std430, binding = 5) buffer PartIndices { uint particleIndices[]; };
layout(std430, binding = 6) buffer CellStart { int cellStartIndices[]; };
layout(std430, binding = 7) buffer CellEnd { int cellEndIndices[]; };
... buffers adicionales según necesidad (densidades, neighbors, etc.)
*/


PBF_GPU_System::PBF_GPU_System()
{
    printf("Sizeof(PBF_GPU_Particle): %d bytes\n", sizeof(PBF_GPU_Particle));

    double massPerParticle = totalMass / static_cast<double>(numParticles);

    particles = std::vector<PBF_GPU_Particle>(numParticles);
    for (int i = 0; i < particles.size(); i++)
    {   // vec4 meta = (x: mass, y: index, z/w: blank atm)
        particles[i].meta = Eigen::Vector4f(massPerParticle, static_cast<float>(i), 0.0f, 0.0f);
        
        // Generating random position particles
        float x = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 4)) - 2.0f;
        float y = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 4)) - 2.0f;
        float z = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 4)) - 2.0f;
        Eigen::Vector4f rndPosition(x, y, z, 1.0f);
        particles[i].x = rndPosition;
        particles[i].p = rndPosition;

        particles[i].v = Eigen::Vector4f::Zero();

        // Color aleatorio
        float r = static_cast<float>(rand()) / RAND_MAX;
        float g = static_cast<float>(rand()) / RAND_MAX;
        float b = static_cast<float>(rand()) / RAND_MAX;
        particles[i].color = Eigen::Vector4f(r, g, b, 1.0f);
    }

    InitSSBOs();
}

void PBF_GPU_System::InitSSBOs()
{
    // (1) PARTICLES SSBO
    glGenBuffers(1, &ssboParticles);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboParticles);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
        sizeof(PBF_GPU_Particle) * particles.size(),
        particles.data(),
        GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboParticles);

    // (2) Cell indices por partícula (keys)
    glGenBuffers(1, &ssboCellIndices);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboCellIndices);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint) * numParticles, nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboCellIndices);

    // (3) Particle indices [0, 1, ..., N-1] (values)
    glGenBuffers(1, &ssboParticleIndices);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboParticleIndices);
    std::vector<GLuint> indices(numParticles);
    for (int i = 0; i < numParticles; ++i) indices[i] = i;
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint) * numParticles, indices.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboParticleIndices);

    // (4) Buffers auxiliares para salida de ordenamiento (double buffer)
    glGenBuffers(1, &ssboKeysOut);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboKeysOut);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint) * numParticles, nullptr, GL_DYNAMIC_DRAW);

    glGenBuffers(1, &ssboValuesOut);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboValuesOut);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint) * numParticles, nullptr, GL_DYNAMIC_DRAW);

    // (5) Bit mask buffer
    glGenBuffers(1, &ssboBitMask);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboBitMask);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint) * numParticles, nullptr, GL_DYNAMIC_DRAW);

    // (6) Scan buffer
    glGenBuffers(1, &ssboScan);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboScan);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint) * numParticles, nullptr, GL_DYNAMIC_DRAW);

    // (7) Bit counter (1 uint)
    glGenBuffers(1, &ssboBitCounter);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboBitCounter);
    GLuint zero = 0;
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint), &zero, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, ssboBitCounter);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}


void PBF_GPU_System::Step()
{
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
}
