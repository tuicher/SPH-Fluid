// PBF_Particle.h
#pragma once

#include "../support/Common.h"

struct PBF_Particle
{
    int    i;
    Scalar m;
    Vec3   x;
    Vec3   v;
    Vec3   p;

    Eigen::Vector3f color = Eigen::Vector3f(0.0f, 0.0f, 1.0f);
};
