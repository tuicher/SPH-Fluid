// SPH_Particle.h
#pragma once
#include <Eigen/Core>

typedef unsigned int uint;

struct Particle
{
	uint id;
	Eigen::Vector3f pos;
	Eigen::Vector3f vel;
	Eigen::Vector3f acc;
	Eigen::Vector3f ev;

	Eigen::Vector3f color;

	float dens;
	float pres;

	float surf_norm;

	Particle* next;
};