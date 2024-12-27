// Mesh.cpp
#include "Mesh.h"

Mesh::Mesh()
{
    // Por defecto, si gustas, color blanco
    m_Color = { 1.0f, 1.0f, 1.0f };
}

Mesh::~Mesh()
{
    // Destruir buffers si están creados
    if (m_VBO) glDeleteBuffers(1, &m_VBO);
    if (m_EBO) glDeleteBuffers(1, &m_EBO);
    if (m_VAO) glDeleteVertexArrays(1, &m_VAO);
}

void Mesh::Setup()
{
    // 1) El hijo rellena m_Vertices, m_Normals, m_Indices
    BuildGeometry();

    // 2) Crear el array intercalado: [pos, normal]
    std::vector<float> interleaved;
    interleaved.reserve(m_Vertices.size() * 6);
    for (size_t i = 0; i < m_Vertices.size(); ++i)
    {
        // pos
        interleaved.push_back(m_Vertices[i].x());
        interleaved.push_back(m_Vertices[i].y());
        interleaved.push_back(m_Vertices[i].z());
        // normal
        interleaved.push_back(m_Normals[i].x());
        interleaved.push_back(m_Normals[i].y());
        interleaved.push_back(m_Normals[i].z());
    }

    // 3) Generar VAO, VBO, EBO
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO);

    glBindVertexArray(m_VAO);

    // Subir VBO
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER,
        interleaved.size() * sizeof(float),
        interleaved.data(),
        GL_STATIC_DRAW);

    // Subir EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        m_Indices.size() * sizeof(unsigned int),
        m_Indices.data(),
        GL_STATIC_DRAW);

    // Atributo de posición (location=0)
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        6 * sizeof(float),
        (void*)0
    );
    glEnableVertexAttribArray(0);

    // Atributo de normal (location=1)
    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        6 * sizeof(float),
        (void*)(3 * sizeof(float))
    );
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void Mesh::Draw()
{
    glBindVertexArray(m_VAO);
    glDrawElements(GL_TRIANGLES,
        static_cast<GLsizei>(m_Indices.size()),
        GL_UNSIGNED_INT,
        0);
    glBindVertexArray(0);
}