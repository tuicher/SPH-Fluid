#ifndef MATH_HELPER_H
#define MATH_HELPER_H

#include <Eigen/Geometry>

using uint = unsigned int;
using Vector2f = Eigen::Vector2f;
using Vector3f = Eigen::Vector3f;
using Vector4f = Eigen::Vector4f;
using Quaternionf = Eigen::Quaternionf;
using Affine3f = Eigen::Affine3f;
using Matrix3f = Eigen::Matrix3f;
using Matrix4f = Eigen::Matrix4f;
using Translation3f = Eigen::Translation3f;

typedef Vector4f Color;

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif // !M_PI

#endif // !MATH_HELPER_H


