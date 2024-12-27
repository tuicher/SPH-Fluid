// Sphere.cpp
#include "Sphere.h"

#include "../support/Common.h"

Sphere::Sphere(float radius, int sectors, int stacks)
    : m_Radius(radius)
    , m_Sectors(sectors)
    , m_Stacks(stacks)
{
    // color verde por defecto
    m_Color = { 0.0f, 1.0f, 0.0f };
}

Sphere::~Sphere()
{
    if (m_VBO)  glDeleteBuffers(1, &m_VBO);
    if (m_EBO)  glDeleteBuffers(1, &m_EBO);
    if (m_VAO)  glDeleteVertexArrays(1, &m_VAO);
}

void Sphere::Setup()
{
    // Construimos vértices e índices
    BuildSphereGeometry();

    // Generar VAO, VBO, EBO
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO);

    glBindVertexArray(m_VAO);

    // Subir vértices
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, m_Vertices.size() * sizeof(float), 
                 m_Vertices.data(), GL_STATIC_DRAW);

    // Subir índices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_Indices.size() * sizeof(unsigned int), 
                 m_Indices.data(), GL_STATIC_DRAW);

    // Atributo posición (asumiendo solo xyz)
    // stride: 3 floats = 3 * sizeof(float)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void Sphere::Draw()
{
    glBindVertexArray(m_VAO);
    glDrawElements(GL_TRIANGLES, (GLsizei)m_Indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

/**
 * @brief Genera los vértices y los índices de la esfera usando las fórmulas paramétricas.
 *        Almacena todo en m_Vertices y m_Indices.
 */
void Sphere::BuildSphereGeometry()
{
    // Limpiamos para evitar sobreescrituras si se llama más de una vez
    m_Vertices.clear();
    m_Indices.clear();

    // Paso angular
    float sectorStep = 2.0f * static_cast<float>(M_PI) / static_cast<float>(m_Sectors);
    float stackStep  = static_cast<float>(M_PI) / static_cast<float>(m_Stacks);

    for (int i = 0; i <= m_Stacks; ++i)
    {
        float stackAngle = static_cast<float>(M_PI) / 2.0f - (float)i * stackStep; 
        // Va de +pi/2 a -pi/2
        float xy = m_Radius * cosf(stackAngle); 
        float z  = m_Radius * sinf(stackAngle);

        for (int j = 0; j <= m_Sectors; ++j)
        {
            float sectorAngle = (float)j * sectorStep; // 0 -> 2pi

            // Coordenadas cartesianas
            float x = xy * cosf(sectorAngle);
            float y = xy * sinf(sectorAngle);

            // Insertamos en el vector de vértices
            m_Vertices.push_back(x);
            m_Vertices.push_back(y);
            m_Vertices.push_back(z);
        }
    }

    // Generar índices
    // Cada stack se conecta con la siguiente, y cada sector con el siguiente
    // i ~ stack, j ~ sector
    for (int i = 0; i < m_Stacks; ++i)
    {
        int k1 = i * (m_Sectors + 1);
        int k2 = k1 + m_Sectors + 1;

        for (int j = 0; j < m_Sectors; ++j, ++k1, ++k2)
        {
            // Triángulo 1
            if (i != 0)
            {
                m_Indices.push_back(k1);
                m_Indices.push_back(k2);
                m_Indices.push_back(k1 + 1);
            }
            // Triángulo 2
            if (i != (m_Stacks - 1))
            {
                m_Indices.push_back(k1 + 1);
                m_Indices.push_back(k2);
                m_Indices.push_back(k2 + 1);
            }
        }
    }
}
