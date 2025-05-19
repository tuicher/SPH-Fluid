// PBF_GPU_System.h

#include <glad/glad.h>
#include <iostream>
#include <numeric>
#include <cfloat>
#include <algorithm>

#include "PBF_GPU_Particle.h"
#include "../graphics/ComputeShader.h"

#define DEBUG

class PBF_GPU_System
{
private:
	// Simulation params
	const GLuint numParticles = 18'000;
	const int numSubSteps = 5;
	const int numIter = 2;
	const double timeStep = 1.0 / 144.0;
	const double radius = 0.40;
	const double restDensity = 1000.0;
	const double epsilon = 1e+05;
	const double damping = 0.999;
	const double viscosity = 0.050;
	const double totalMass = 3000.0;
	const double massPerParticle = totalMass / numParticles;
	const Eigen::Vector3f gravity = Eigen::Vector3f(0, -9.81f, 0);
	//Eigen::Vector3f gridOrigin = Eigen::Vector3f(-2.4f, -0.4f, -2.4f);
	Eigen::Vector3f gridOrigin = Eigen::Vector3f::Zero();
	
	// Kernels Consts
	const GLuint workGroup = 128;
	const GLuint numWorkGroups = (numParticles + workGroup - 1) / workGroup;
	const GLint gridRes = 64;
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

	GLuint ssboDensity;
	GLuint ssboDeltaV;

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
	
	ComputeShader updateVelocity;
	ComputeShader resolveCollisions;

	ComputeShader computeDensity;
	ComputeShader applyViscosity;

	const int initStart = INT_MAX;   //  0x7FFFFFFF
	const int initEnd = -1;

	void InitParticles();
	void SetParticlesColors();
	void InitSSBOs();
	void InitComputeShaders();

	void UpdateGrid();

public:
	PBF_GPU_System();
	~PBF_GPU_System();

	int currentGridRes = gridRes;

	void Init();
	void Step();
	//void Step(float timeStep);
	//void Test();
	void Test(int n);

	void ResizeCellBuffers(GLuint newTotCells);

	inline GLuint GetParticlesSSBO() const	{ return ssboParticles; }
	inline GLuint GetNumParticles() const	{ return numParticles; }

	inline int NextPowerOfTwo(int v) {
		v--;
		v |= v >> 1;
		v |= v >> 2;
		v |= v >> 4;
		v |= v >> 8;
		v |= v >> 16;
		v++;
		return v;
	}
};