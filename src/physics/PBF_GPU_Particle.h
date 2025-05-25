// PBF_GPU_Particle.h
#pragma once

#include <Eigen/Core>


// GPU-aligned particle data structure for SSBO usage.
// This struct mirrors the layout in GLSL compute shaders.
struct PBF_GPU_Particle
{
    EIGEN_ALIGN16 Eigen::Vector4f x;      // Current position (xyz), w unused
    EIGEN_ALIGN16 Eigen::Vector4f v;      // Velocity (xyz), w unused
    EIGEN_ALIGN16 Eigen::Vector4f p;      // Predicted position (xyz), w unused
    EIGEN_ALIGN16 Eigen::Vector4f color;  // RGBA color
    EIGEN_ALIGN16 Eigen::Vector4f meta;   // x: mass, y: index, z/w: optional (density, flags, etc.)
};

/// GLSL Version:
// struct Particle{
//    vec4 x;
//    vec4 v;
//    vec4 p;
//    vec4 color;
//    vec4 meta; // meta.x = masa, meta.y = índice
//};