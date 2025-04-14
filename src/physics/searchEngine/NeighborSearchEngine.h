// NeighborSearchEngine.h
#pragma once

#include "../PBF_Particle.h"
#include "../../support/Common.h"

#include <map>
#include <tuple>
#include <unordered_map>
#include <vector>

class NeighborSearchEngine
{
public:
    NeighborSearchEngine(const Scalar radius, const std::vector<PBF_Particle>& particles)
        : m_radius(radius), m_particles(particles) {}

    virtual void searchNeighbors() = 0;

    inline const std::vector<int>& retrieveNeighbors(const int index) const
    {
        return m_neighbors_list[index];
    }

    inline int getNumParticles() const { return m_neighbors_list.size(); }
protected:
    std::vector<std::vector<int>>       m_neighbors_list;

    const Scalar                        m_radius;
    const std::vector<PBF_Particle>&    m_particles;
};