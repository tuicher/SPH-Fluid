// Common.h
#pragma once
#include <Eigen/Core>

// DEFINES
#define M_PI 3.14159265358979323846
#define INF 1E-12f
#define BOUNDARY 0.0001f

static constexpr float DEG2RAD = 3.14159265358979323846f / 180.0f;

// USING
using Scalar = double;
using Vec3 = Eigen::Matrix<Scalar, 3, 1>;
using VecX = Eigen::Matrix<Scalar, Eigen::Dynamic, 1>;
using MatX = Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>;