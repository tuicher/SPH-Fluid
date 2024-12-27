// Sphere.h
#pragma once
#include "Mesh.h"
#include <vector>

/**
 * @brief Clase para representar la malla de una esfera.
 */
class Sphere : public Mesh
{
public:
    /**
     * @param radius Radio de la esfera.
     * @param sectors Número de divisiones horizontales (meridianos).
     * @param stacks Número de divisiones verticales (paralelos).
     */
    Sphere(float radius = 1.0f, int sectors = 16, int stacks = 16);
    virtual ~Sphere();

    void Setup() override;
    void Draw() override;

private:
    float m_Radius;
    int   m_Sectors;
    int   m_Stacks;

    // Guardaremos temporalmente los datos de la esfera
    // antes de subirlos a la GPU.
    // (Podrías hacerlos static si siempre es igual).
    std::vector<float> m_Vertices;
    std::vector<unsigned int> m_Indices;

    void BuildSphereGeometry();
};

