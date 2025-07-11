// PBF_GPU_System.h

#include <glad/glad.h>
#include <iostream>
#include <iomanip>
#include <numeric>
#include <cfloat>
#include <algorithm>
#include <omp.h>

#include "PBF_GPU_Particle.h"
#include "../graphics/ComputeShader.h"

//#define DEBUG
//#define AABB

class PBF_GPU_System
{
private:
	// Simulation params
	const GLuint numParticles = 75'000;
	//const GLuint numParticles = 18'000;
	const int numRelaxSteps = 200;
	const int numSubSteps = 3;
	const int numIter = 2;
	const float timeStep = 1.0f / 140.0f;
	const float subTimeStep = timeStep / numSubSteps;
	const double radius = 0.1;
	const double restDensity = 1000.0;
	const double epsilon = 1e05;
	const double damping = 0.999;
	const double viscosity = 0.010;
	const double totalMass = 4000.0;
	//const double totalMass = 1081.0;
	const double massPerParticle = totalMass / numParticles;
	const Eigen::Vector3f gravity = Eigen::Vector3f(0.f, -9.81f, 0.f);
	Eigen::Vector3f gridOrigin = Eigen::Vector3f( 0.f, 5.f, 0.f);
	
	// Kernels Consts
	const GLuint workGroup = 128;
	const GLuint numWorkGroups = (numParticles + workGroup - 1) / workGroup;
	Eigen::Array3i gridRes = Eigen::Array3i(600,80,600);
	GLuint totCells = gridRes.prod();
	const float cellSize = 0.1f;
#ifdef AABB
	const Eigen::Vector3f MinBound = Eigen::Vector3f(-2.f, 0.0f, -2.f);
	const Eigen::Vector3f MaxBound = Eigen::Vector3f( 2.f, 30.0f, 2.f);
#else
	const Eigen::Vector3f SphereCenter = Eigen::Vector3f(0.f, 1.f, 0.f);
	const float SphereRadius = 2.f;
#endif	
	const bool verbose = false;

	std::vector<PBF_GPU_Particle> particles;

	// SSBOs
	GLuint ssboParticles;		//  0
	GLuint ssboCellKey;			//  1
	GLuint ssboParticleIdx;		//  2
		// -- Radix Short
	GLuint ssboBits;			//  3
	GLuint ssboScan;			//  4
	GLuint ssboSums;			//  5
	GLuint ssboOffsets;			//  6
	GLuint ssboKeysTmp;			//  7
	GLuint ssboValsTmp;			//  8
	GLuint ssboCellStart;		//  9
	GLuint ssboCellEnd;			// 10
	GLuint ssboLambda;			// 11
	GLuint ssboDeltaP;			// 12
	GLuint ssboDensity;			// 13
	GLuint ssboDeltaV;			// 14

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

	ComputeShader resetVelocity;

	const int initStart = INT_MAX;   //  0x7FFFFFFF
	const int initEnd = -1;

	void InitParticles();
	void SetParticlesColors();
	void InitSSBOs();
	void InitComputeShaders();
	void InitSimulation();
	void UpdateGrid();

	void PrintTimes() const;

	// Time measures
	double cpuUpdateGrid_ms = 0.0;

	double gpuIntegrate_ms = 0.0;
	double gpuHash_ms = 0.0;
	double gpuRadixShort_ms = 0.0;
	double gpuFindCellBounds_ms = 0.0;
	double gpuPBF_ms = 0.0;
	double gpuUpdateVelocity_ms = 0.0;
	double gpuXSPH_ms = 0.0;
	double gpuViscosity_ms = 0.0;
	double gpuCollisions_ms = 0.0;


public:
	PBF_GPU_System();
	~PBF_GPU_System();

	void Init();
	void Step();
	void Step(float timeStep);
	//void Test();
	void Test(int n);

	int currentTotCells = totCells;

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