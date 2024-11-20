#include "./GLADhelper.h"

GLAD_Helper::GLAD_Helper()
    : projectionMode(PM_Normalized), viewportWidth(800), viewportHeight(600)
{
    currentMatrix = Matrix4f::Identity();
}


void GLAD_Helper::setProjectionMode2D(ProjectionMode2D pm, int width, int height)
{
    projectionMode = pm;
    viewportWidth = width;
    viewportHeight = height;

    if (pm == PM_Normalized)
    {
        currentMatrix = Matrix4f::Identity();
    }
    else if (pm == PM_ViewPort) 
    {
        currentMatrix = Matrix4f::Identity();
        currentMatrix(0, 0) = 2.0f / width;
        currentMatrix(1, 1) = -2.0f / height;
        currentMatrix(0, 3) = -1.0f;
        currentMatrix(1, 3) = 1.0f;
    }
}

void GLAD_Helper::loadMatrix(const Matrix4f& mat, GLuint uniformLocation)
{
    currentMatrix = mat;
    glUniformMatrix4fv(uniformLocation, 1, GL_FALSE, currentMatrix.data());
}

void GLAD_Helper::multMatrix(const Matrix4f& mat, GLuint uniformLocation)
{
    currentMatrix *= mat;
    glUniformMatrix4fv(uniformLocation, 1, GL_FALSE, currentMatrix.data());
}

void GLAD_Helper::pushMatrix(const Matrix4f& mat)
{
    matrixStack.push_back(currentMatrix);
    currentMatrix *= mat;
}

void GLAD_Helper::popMatrix()
{
    if (matrixStack.empty())
    {
        throw std::runtime_error("Matrix stack underflow");
    }
    currentMatrix = matrixStack.back();
    matrixStack.pop_back();
}