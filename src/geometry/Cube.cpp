// Cube.cpp
#include "Cube.h"
#include <Eigen/Geometry>
#include <vector>
#include <iostream>

Cube::Cube()
{
    // Color rojo por defecto
    m_Color = { 1.0f, 0.0f, 0.0f };
}

void Cube::BuildGeometry()
{
    m_Vertices.clear();
    m_Normals.clear();
    m_Indices.clear();

    // Datos de vértices para un cubo centrado en (0,0,0)
    m_Vertices = {
       Eigen::Vector3f(-0.5f,  0.5f,  0.5f),    // 0
       Eigen::Vector3f( 0.5f,  0.5f,  0.5f),    // 1
       Eigen::Vector3f( 0.5f, -0.5f,  0.5f),    // 2
       Eigen::Vector3f(-0.5f, -0.5f,  0.5f),    // 3
       Eigen::Vector3f(-0.5f,  0.5f, -0.5f),    // 4
       Eigen::Vector3f( 0.5f,  0.5f, -0.5f),    // 5
       Eigen::Vector3f( 0.5f, -0.5f, -0.5f),    // 6
       Eigen::Vector3f(-0.5f, -0.5f, -0.5f)     // 7
    };

    m_Indices = {
        0,1,2,  2,3,0,  // front
        1,5,6,  6,2,1,  // right
        5,4,7,  7,6,5,  // back
        4,0,3,  3,7,4,  // left
        4,5,1,  1,0,4,  // top
        3,2,6,  6,7,3   // bottom
    };

    m_Normals = {
       Eigen::Vector3f(-1,  1,  1).normalized(), // 0
       Eigen::Vector3f( 1,  1,  1).normalized(), // 1
       Eigen::Vector3f( 1, -1,  1).normalized(), // 2
       Eigen::Vector3f(-1, -1,  1).normalized(), // 3
       Eigen::Vector3f(-1,  1, -1).normalized(), // 4
       Eigen::Vector3f( 1,  1, -1).normalized(), // 5
       Eigen::Vector3f( 1, -1, -1).normalized(), // 6
       Eigen::Vector3f(-1, -1, -1).normalized()  // 7
    };
}
