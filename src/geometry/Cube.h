// Cube.h
#pragma once
#include "Mesh.h"

class Cube : public Mesh
{
public:
    Cube();
    virtual ~Cube() = default;

private:
    void BuildGeometry() override;
};
