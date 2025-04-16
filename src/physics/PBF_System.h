// PBF_System.h
#pragma once

#include <omp.h>

#include "PBF_Particle.h"
#include "../support/Common.h"
#include "./searchEngine/HashGrid.h"
#include "./maths/Kernel.h"

class PBF_System
{
private:
	// Data
	std::vector<PBF_Particle> particles;

	// Simulation params
	const int numParticles = 10800;
	//const int numParticles = 5400;
	const int numSubSteps = 5;
	const int numIter = 2;

	const Scalar timeStep = 1.0 / 90.0;
	const Scalar radius = 0.10;
	const Scalar restDensity = 1000.0;
	const Scalar epsilon = 1e+05;
	const Scalar damping = 0.999;
	const Scalar viscosity = 0.050;

	const bool verbose = false;

	HashGrid neighborSearchEngine;
	
	void InitSystem();
	void SetParticlesColors();
	Scalar CalcDensity(const int target_index);
	Scalar CalcConstraint(const int target_index);
	Vec3 CalcGradConstraint(const int target_index, const int var_index);

	void PrintAverageNumNeighbors();
	void PrintAverageDensity();

public:
	PBF_System();
	~PBF_System();

	inline const std::vector<PBF_Particle>& getParticles() const { return particles; }
	inline int getNumParticles() const { return particles.size(); }
	inline const PBF_Particle& getParticle(int index) const { return particles[index]; }
	
	void AnimationStep();
	void Step(const Scalar dt);
private:
};