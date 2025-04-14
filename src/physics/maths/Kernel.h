// Kernel.h
#pragma once

#include "../../support/Common.h"

Scalar calcPoly6Kernel(const Vec3& r, const Scalar h);
Vec3 calcGradPoly6Kernel(const Vec3& r, const Scalar h);

Scalar calcSpikyKernel(const Vec3& r, const Scalar h);
Vec3 calcGradSpikyKernel(const Vec3& r, const Scalar h);

constexpr auto CalcKernel = calcPoly6Kernel;
constexpr auto CalcGradKernel = calcGradSpikyKernel;