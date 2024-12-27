// Mesh.h
#pragma once
#include <glad/glad.h>
#include <Eigen/Core>

/**
 * @brief Clase abstracta para representar una malla.
 *        Ofrece una interfaz para configurar y dibujar una malla.
 */
class Mesh
{
public:
    virtual ~Mesh() = default;

    /**
     * @brief Configura la malla: genera buffers, sube datos de vértices e índices a la GPU, etc.
     */
    virtual void Setup() = 0;

    /**
     * @brief Llama a glDraw* correspondiente para dibujar la malla en pantalla.
     */
    virtual void Draw() = 0;

    void SetColor(const Eigen::Vector3f& color) { m_Color = color; }
    const Eigen::Vector3f& GetColor() const { return m_Color; }

protected:
    GLuint m_VAO = 0;
    GLuint m_VBO = 0;
    GLuint m_EBO = 0;

    // Color base de la malla
    Eigen::Vector3f m_Color = { 1.0f, 1.0f, 1.0f };  // Por defecto blanco
};

