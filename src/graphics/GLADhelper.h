#ifndef GLAD_HELPER_H
#define GLAD_HELPER_H

#include <glad/glad.h>
#include <vector>
#include <stdexcept>

#include "../support/math_helper.h" 

class GLAD_Helper
{
public:
    GLAD_Helper();
    ~GLAD_Helper() {};
    
    // Defines 2D projection modes: normalized (-1 to 1) or viewport-based (0 to width/height).
    enum ProjectionMode2D {PM_Normalized = 1, PM_ViewPort = 2};

    void setProjectionMode2D(ProjectionMode2D pm, int width, int height);
    void loadMatrix(const Matrix4f& mat, GLuint uniformLocation);
    void multMatrix(const Matrix4f& mat, GLuint uniformLocation);
    void pushMatrix(const Matrix4f& mat);
    void popMatrix();
    
    void drawVector(const Vector3f& position, const Vector3f& vec, const Color& color, float aspect = 50.0f);
    void drawUnitCube();
    void drawUnitSphere(int level = 0);

private:
    std::vector<Matrix4f> matrixStack;  // Simulate matrices' stack
    Matrix4f currentMatrix;
    ProjectionMode2D projectionMode;
    int viewportWidth, viewportHeight;

    void updateCurrentMatrix();

};

#endif // GLAD_HELPER_H