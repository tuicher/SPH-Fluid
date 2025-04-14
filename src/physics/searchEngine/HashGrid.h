//HashGrid.h
#pragma once

#include "NeighborSearchEngine.h"

class HashGrid : public NeighborSearchEngine
{
public:
	HashGrid(const Scalar radius, const std::vector<PBF_Particle>& particles);

	void searchNeighbors() override;
private:
	using GridIndex = std::tuple<int, int, int>;

	GridIndex calcGridIndex(const Scalar radius, const MatX& positions, const int target_particle_index);

	int convertGridIndexToArrayIndex(const GridIndex& index);

	std::unordered_map<int, std::vector<int>> constructGridCells(const Scalar radius, const MatX& positions);

	const int  k_n_x = 128; // Sufficiently large value
	const int  k_n_y = 128;
	const int  k_n_z = 128;
	const Vec3 k_grid_center = Vec3(0.0, 1.0, 0.0);
};