// PBF_GPU_System.h

#include <glad/glad.h>

#include "PBF_GPU_Particle.h"

class PBF_GPU_System
{
private:
	// Simulation params
	const int numParticles = 10800;
	const int numSubSteps = 5;
	const int numIter = 2;
	const double timeStep = 1.0 / 90.0;
	const double radius = 0.10;
	const double restDensity = 1000.0;
	const double epsilon = 1e+05;
	const double damping = 0.999;
	const double viscosity = 0.050;
	const double totalMass = 3000.0;
	const bool verbose = false;

	std::vector<PBF_GPU_Particle> particles;

	// SSBOs
	GLuint ssboParticles;
	GLuint ssboCellIndices;
	GLuint ssboParticleIndices;

	GLuint ssboKeys;       // copia de cellIndices para ordenar
	GLuint ssboValues;     // copia de particleIndices para ordenar
	GLuint ssboKeysOut;    // sorted output
	GLuint ssboValuesOut;

	GLuint ssboBitMask;    // bits extraídos por radix
	GLuint ssboScan;       // scan intermedio
	GLuint ssboBitCounter; // para contar bits == 1 (atomic)

	// Compute shader programs
	GLuint programIntegrate;
	GLuint programAssignCells;
	GLuint programExtractBit;
	GLuint programScan;
	GLuint programCountOnes;
	GLuint programReorder;

	void InitSSBOs();

public:
	PBF_GPU_System();
	~PBF_GPU_System();

	void Step();
};