// Mesh.h
#pragma once
#include <vector>
#include <Eigen/Core>
#include <glad/glad.h>

/**
 * @brief Clase base que representa una malla.
 *        Almacena contenedores de vértices, normales e índices.
 *        Los hijos deben implementar BuildGeometry().
 */
class Mesh
{
public:
    virtual ~Mesh();

    // Configura la malla (crea buffers, sube datos). 
    // Llama internamente a BuildGeometry() para que el hijo rellene los vectores.
    void Setup();

    // Llama a glDrawElements o lo que corresponda
    virtual void Draw();

    // Color base que usas en el shading
    void SetColor(const Eigen::Vector3f& color) { m_Color = color; }
    const Eigen::Vector3f& GetColor() const { return m_Color; }

    inline GLuint GetVAO() { return m_VAO; }
    inline GLuint GetVBO() { return m_VBO; }
    inline GLuint GetEBO() { return m_EBO; }

    inline GLuint GetNumVertex() { return m_Vertices.size(); }
    inline GLuint GetNumNormals() { return m_Normals.size(); }
    inline GLuint GetNumIndex() { return m_Indices.size(); }

protected:
    Mesh();  // protected para que sólo se instancie derivando

    // Método que debe implementar cada hijo para rellenar
    // m_Vertices, m_Normals y m_Indices.
    virtual void BuildGeometry() = 0;

    // Contenedores con la info de la malla
    std::vector<Eigen::Vector3f> m_Vertices;
    std::vector<Eigen::Vector3f> m_Normals;
    std::vector<unsigned int>    m_Indices;

    // OpenGL buffers
    GLuint m_VAO = 0;
    GLuint m_VBO = 0;
    GLuint m_EBO = 0;

    // Color del objeto
    Eigen::Vector3f m_Color = { 1.0f, 1.0f, 1.0f };
};

