// Sphere.cpp
#include "Sphere.h"
#include "../support/Common.h"

Sphere::Sphere(float radius, int sectors, int stacks)
    : m_Radius(radius)
    , m_Sectors(sectors)
    , m_Stacks(stacks)
{
    // color verde, por ejemplo
    m_Color = { 0.0f, 1.0f, 0.0f };
}

void Sphere::BuildGeometry()
{
    m_Vertices.clear();
    m_Normals.clear();
    m_Indices.clear();

    float sectorStep = 2.0f * static_cast<float>(M_PI) / (float)m_Sectors;
    float stackStep = static_cast<float>(M_PI) / (float)m_Stacks;

    // Generar vértices
    for (int i = 0; i <= m_Stacks; ++i)
    {
        float stackAngle = (static_cast<float>(M_PI) / 2.0f) - (float)i * stackStep;
        float xy = m_Radius * cosf(stackAngle);
        float z = m_Radius * sinf(stackAngle);

        for (int j = 0; j <= m_Sectors; ++j)
        {
            float sectorAngle = (float)j * sectorStep;
            float x = xy * cosf(sectorAngle);
            float y = xy * sinf(sectorAngle);

            Eigen::Vector3f v(x, y, z);
            m_Vertices.push_back(v);

            // Normal = posición normalizada (esfera centrada en el origen)
            m_Normals.push_back(v.normalized());
        }
    }

    // Generar índices
    int columns = m_Sectors + 1;
    for (int i = 0; i < m_Stacks; ++i)
    {
        int k1 = i * columns;
        int k2 = k1 + columns;

        for (int j = 0; j < m_Sectors; ++j, ++k1, ++k2)
        {
            if (i != 0)
            {
                m_Indices.push_back(k1);
                m_Indices.push_back(k2);
                m_Indices.push_back(k1 + 1);
            }
            if (i != (m_Stacks - 1))
            {
                m_Indices.push_back(k1 + 1);
                m_Indices.push_back(k2);
                m_Indices.push_back(k2 + 1);
            }
        }
    }
}
