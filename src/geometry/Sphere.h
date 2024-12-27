// Sphere.h
#pragma once
#include "Mesh.h"

class Sphere : public Mesh
{
public:
    Sphere(float radius = 1.0f, int sectors = 16, int stacks = 16);
    virtual ~Sphere() = default;

private:
    void BuildGeometry() override;

    float m_Radius;
    int   m_Sectors;
    int   m_Stacks;
};
