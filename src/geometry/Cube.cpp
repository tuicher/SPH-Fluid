// Cube.cpp
#include "Cube.h"
#include <vector>
#include <iostream>

Cube::Cube()
{
    // Color rojo por defecto
    m_Color = { 1.0f, 0.0f, 0.0f };
}

Cube::~Cube()
{
    // Liberamos los buffers
    if (m_VBO)  glDeleteBuffers(1, &m_VBO);
    if (m_EBO)  glDeleteBuffers(1, &m_EBO);
    if (m_VAO)  glDeleteVertexArrays(1, &m_VAO);
}

void Cube::Setup()
{
    // Datos de vértices para un cubo centrado en (0,0,0)
    float cubeVertices[] = {
        // Cara frontal (z=+0.5)
        -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        // Cara trasera (z=-0.5)
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f
    };

    unsigned int cubeIndices[] = {
        // Cara frontal
        0, 1, 2,
        2, 3, 0,
        // Cara derecha
        1, 5, 6,
        6, 2, 1,
        // Cara trasera
        5, 4, 7,
        7, 6, 5,
        // Cara izquierda
        4, 0, 3,
        3, 7, 4,
        // Cara superior
        3, 2, 6,
        6, 7, 3,
        // Cara inferior
        4, 5, 1,
        1, 0, 4
    };

    // Generar VAO, VBO, EBO
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO);

    // Enlazar VAO
    glBindVertexArray(m_VAO);

    // Subir vértices
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    // Subir índices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW);

    // Atributo de posición
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Desenlazar
    glBindVertexArray(0);
}

void Cube::Draw()
{
    glBindVertexArray(m_VAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
