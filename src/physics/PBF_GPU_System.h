// PBF_GPU_System.h

#include <glad/glad.h>
#include <iostream>
#include <numeric>

#include "PBF_GPU_Particle.h"
#include "../graphics/ComputeShader.h"

#define DEBUG

class PBF_GPU_System
{
private:
	// Simulation params
	
	const GLuint numParticles = 5400 * 10;
	const int numSubSteps = 5;
	const int numIter = 2;
	const double timeStep = 1.0 / 90.0;
	const double radius = 0.10;
	const double restDensity = 1000.0;
	const double epsilon = 1e-4;
	const double damping = 0.999;
	const double viscosity = 0.050;
	const double totalMass = 3000.0;
	const double massPerParticle = totalMass / numParticles;
	const Eigen::Vector3f gravity = Eigen::Vector3f(0, -9.81f, 0);
	
	// Kernels Consts
	const GLuint workGroup = 128;
	const GLuint numWorkGroups = (numParticles + workGroup - 1) / workGroup;
	const GLint gridRes = 11;
	const GLuint totCells = gridRes * gridRes * gridRes;
	const float cellSize = 0.4f;
	
	const bool verbose = false;

	std::vector<PBF_GPU_Particle> particles;

	// SSBOs
	GLuint ssboParticles;		// 0
	GLuint ssboCellKey;			// 1
	GLuint ssboParticleIdx;		// 2
		// -- Radix Short
	GLuint ssboBits;			// 3
	GLuint ssboScan;			// 4
	GLuint ssboSums;			// 5
	GLuint ssboOffsets;			// 6
	GLuint ssboKeysTmp;			// 7
	GLuint ssboValsTmp;			// 8

	GLuint ssboCellStart;		// 9
	GLuint ssboCellEnd;			// 10
	
	GLuint ssboLambda;			// 11
	GLuint ssboDeltaP;			// 12

	// Compute Shaders
	ComputeShader integrate;

	ComputeShader assign;

	ComputeShader rsExtract;
	ComputeShader rsScan;
	ComputeShader rsAddOffset;
	ComputeShader rsReorder;

	ComputeShader findBounds;

	ComputeShader computeLambda;
	ComputeShader computeDeltaP;
	ComputeShader applyDeltaP;


	const int initStart = INT_MAX;   //  0x7FFFFFFF
	const int initEnd = -1;

	void InitParticles();
	void InitSSBOs();
	void InitComputeShaders();

public:
	PBF_GPU_System();
	~PBF_GPU_System();

	void Init();
	void Step();
	void Test();
	void Test(int n);
};