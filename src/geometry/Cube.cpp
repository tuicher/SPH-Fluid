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

    //
    // Cada cara tiene 4 vértices y 1 normal plana.
    // Terminaremos con 24 vértices y 36 índices (6 caras × 2 triángulos × 3 vértices).
    //

    // -----------  CARA FRONTAL (z = +0.5)  -----------
    {
        // Normal de la cara frontal
        Eigen::Vector3f normalFront(0.0f, 0.0f, 1.0f);

        // Antes de añadir vértices, calculamos offset (cuántos llevamos)
        int offset = (int)m_Vertices.size();

        // Vértices de la cara frontal
        m_Vertices.push_back(Eigen::Vector3f(-0.5f, -0.5f, 0.5f)); // v0
        m_Vertices.push_back(Eigen::Vector3f(0.5f, -0.5f, 0.5f)); // v1
        m_Vertices.push_back(Eigen::Vector3f(0.5f, 0.5f, 0.5f)); // v2
        m_Vertices.push_back(Eigen::Vector3f(-0.5f, 0.5f, 0.5f)); // v3

        // Misma normal para los 4 vértices
        m_Normals.push_back(normalFront);
        m_Normals.push_back(normalFront);
        m_Normals.push_back(normalFront);
        m_Normals.push_back(normalFront);

        // Indices (2 triángulos) => (v0, v1, v2) y (v2, v3, v0)
        m_Indices.push_back(offset + 0);
        m_Indices.push_back(offset + 1);
        m_Indices.push_back(offset + 2);

        m_Indices.push_back(offset + 2);
        m_Indices.push_back(offset + 3);
        m_Indices.push_back(offset + 0);
    }

    // -----------  CARA TRASERA (z = -0.5)  -----------
    {
        Eigen::Vector3f normalBack(0.0f, 0.0f, -1.0f);
        int offset = (int)m_Vertices.size();

        m_Vertices.push_back(Eigen::Vector3f(0.5f, -0.5f, -0.5f)); // v0
        m_Vertices.push_back(Eigen::Vector3f(-0.5f, -0.5f, -0.5f)); // v1
        m_Vertices.push_back(Eigen::Vector3f(-0.5f, 0.5f, -0.5f)); // v2
        m_Vertices.push_back(Eigen::Vector3f(0.5f, 0.5f, -0.5f)); // v3

        m_Normals.push_back(normalBack);
        m_Normals.push_back(normalBack);
        m_Normals.push_back(normalBack);
        m_Normals.push_back(normalBack);

        m_Indices.push_back(offset + 0);
        m_Indices.push_back(offset + 1);
        m_Indices.push_back(offset + 2);

        m_Indices.push_back(offset + 2);
        m_Indices.push_back(offset + 3);
        m_Indices.push_back(offset + 0);
    }

    // -----------  CARA DERECHA (x = +0.5)  -----------
    {
        Eigen::Vector3f normalRight(1.0f, 0.0f, 0.0f);
        int offset = (int)m_Vertices.size();

        m_Vertices.push_back(Eigen::Vector3f(0.5f, -0.5f, 0.5f)); // v0
        m_Vertices.push_back(Eigen::Vector3f(0.5f, -0.5f, -0.5f)); // v1
        m_Vertices.push_back(Eigen::Vector3f(0.5f, 0.5f, -0.5f)); // v2
        m_Vertices.push_back(Eigen::Vector3f(0.5f, 0.5f, 0.5f)); // v3

        m_Normals.push_back(normalRight);
        m_Normals.push_back(normalRight);
        m_Normals.push_back(normalRight);
        m_Normals.push_back(normalRight);

        m_Indices.push_back(offset + 0);
        m_Indices.push_back(offset + 1);
        m_Indices.push_back(offset + 2);

        m_Indices.push_back(offset + 2);
        m_Indices.push_back(offset + 3);
        m_Indices.push_back(offset + 0);
    }

    // -----------  CARA IZQUIERDA (x = -0.5)  -----------
    {
        Eigen::Vector3f normalLeft(-1.0f, 0.0f, 0.0f);
        int offset = (int)m_Vertices.size();

        m_Vertices.push_back(Eigen::Vector3f(-0.5f, -0.5f, -0.5f)); // v0
        m_Vertices.push_back(Eigen::Vector3f(-0.5f, -0.5f, 0.5f)); // v1
        m_Vertices.push_back(Eigen::Vector3f(-0.5f, 0.5f, 0.5f)); // v2
        m_Vertices.push_back(Eigen::Vector3f(-0.5f, 0.5f, -0.5f)); // v3

        m_Normals.push_back(normalLeft);
        m_Normals.push_back(normalLeft);
        m_Normals.push_back(normalLeft);
        m_Normals.push_back(normalLeft);

        m_Indices.push_back(offset + 0);
        m_Indices.push_back(offset + 1);
        m_Indices.push_back(offset + 2);

        m_Indices.push_back(offset + 2);
        m_Indices.push_back(offset + 3);
        m_Indices.push_back(offset + 0);
    }

    // -----------  CARA SUPERIOR (y = +0.5)  -----------
    {
        Eigen::Vector3f normalTop(0.0f, 1.0f, 0.0f);
        int offset = (int)m_Vertices.size();

        m_Vertices.push_back(Eigen::Vector3f(-0.5f, 0.5f, 0.5f)); // v0
        m_Vertices.push_back(Eigen::Vector3f(0.5f, 0.5f, 0.5f)); // v1
        m_Vertices.push_back(Eigen::Vector3f(0.5f, 0.5f, -0.5f)); // v2
        m_Vertices.push_back(Eigen::Vector3f(-0.5f, 0.5f, -0.5f)); // v3

        m_Normals.push_back(normalTop);
        m_Normals.push_back(normalTop);
        m_Normals.push_back(normalTop);
        m_Normals.push_back(normalTop);

        m_Indices.push_back(offset + 0);
        m_Indices.push_back(offset + 1);
        m_Indices.push_back(offset + 2);

        m_Indices.push_back(offset + 2);
        m_Indices.push_back(offset + 3);
        m_Indices.push_back(offset + 0);
    }

    // -----------  CARA INFERIOR (y = -0.5)  -----------
    {
        Eigen::Vector3f normalBottom(0.0f, -1.0f, 0.0f);
        int offset = (int)m_Vertices.size();

        m_Vertices.push_back(Eigen::Vector3f(-0.5f, -0.5f, -0.5f)); // v0
        m_Vertices.push_back(Eigen::Vector3f(0.5f, -0.5f, -0.5f)); // v1
        m_Vertices.push_back(Eigen::Vector3f(0.5f, -0.5f, 0.5f)); // v2
        m_Vertices.push_back(Eigen::Vector3f(-0.5f, -0.5f, 0.5f)); // v3

        m_Normals.push_back(normalBottom);
        m_Normals.push_back(normalBottom);
        m_Normals.push_back(normalBottom);
        m_Normals.push_back(normalBottom);

        m_Indices.push_back(offset + 0);
        m_Indices.push_back(offset + 1);
        m_Indices.push_back(offset + 2);

        m_Indices.push_back(offset + 2);
        m_Indices.push_back(offset + 3);
        m_Indices.push_back(offset + 0);
    }

    // Ahora tienes 6×4 = 24 vértices y 6×2×3 = 36 índices
    // Todos con normales planas, una para cada cara.
}
