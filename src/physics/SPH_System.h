#pragma once

#include "SPH_Particle.h"
#include "../support/Common.h"

class SPH_System
{
private:
	uint maxParticles;

	float kernel;
	float mass;

	Eigen::Vector3f worldSize;
	float cellSize;
	Eigen::Vector3i gridSize;
	uint totCell;

	Eigen::Vector3f gravity;
	float wallDamping;
	float restDensity;
	float gasConstant;
	float viscosity;
	float timeStep;
	float surfNorm;
	float surfCoe;

	float poly6Value;
	float spikyValue;
	float viscoValue;

	float gradPoly6;
	float lplcPoly6;

	float kernel_2;
	float self_dens;
	float self_lplc_color;

	
	Particle** cell;

public:
	SPH_System();
	~SPH_System();
	void Animation();
	void InitSystem();
	void AddParticle(Eigen::Vector3f pos, Eigen::Vector3f vel);

	Particle* mem;
	uint numParticles;

	uint sys_running;

private:
	void BuildTable();
	void Comp_DensPres();
	void Comp_ForceAdv();
	void Advection();

	Eigen::Vector3i Calc_CellPos(Eigen::Vector3f p);
	uint Calc_CellHash(Eigen::Vector3i cellPos);
};