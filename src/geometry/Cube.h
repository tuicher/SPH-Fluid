// Cube.h
#pragma once
#include "Mesh.h"

/**
 * @brief Clase para representar la malla de un cubo.
 */
class Cube : public Mesh
{
public:
    Cube();
    virtual ~Cube();

    void Setup() override;
    void Draw() override;
};

