// Camera.cpp
#include "Camera.h"

#include "../support/Common.h"

// Constructor con valores por defecto
Camera::Camera()
    : m_Position(0.0f, 0.0f, 3.0f)
    , m_Target(0.0f, 0.0f, 0.0f)
    , m_Up(0.0f, 1.0f, 0.0f)
    , m_FOV(45.0f)
    , m_AspectRatio(4.0f / 3.0f)
    , m_NearPlane(0.1f)
    , m_FarPlane(100.0f)
{
    UpdateViewMatrix();
    UpdateProjectionMatrix();
}

void Camera::SetPosition(const Eigen::Vector3f& position)
{
    m_Position = position;
    UpdateViewMatrix();
}

void Camera::SetTarget(const Eigen::Vector3f& target)
{
    m_Target = target;
    UpdateViewMatrix();
}

void Camera::SetFOV(float fovDegrees)
{
    m_FOV = fovDegrees;
    UpdateProjectionMatrix();
}

void Camera::SetAspectRatio(float aspect)
{
    m_AspectRatio = aspect;
    UpdateProjectionMatrix();
}

void Camera::SetClippingPlanes(float nearPlane, float farPlane)
{
    m_NearPlane = nearPlane;
    m_FarPlane = farPlane;
    UpdateProjectionMatrix();
}

const Eigen::Matrix4f& Camera::GetViewMatrix() const
{
    return m_View;
}

const Eigen::Matrix4f& Camera::GetProjectionMatrix() const
{
    return m_Projection;
}

void Camera::UpdateViewMatrix()
{
    // Usamos la aproximación "lookAt" con Eigen
    // Vector dirección
    Eigen::Vector3f zAxis = (m_Position - m_Target).normalized();
    Eigen::Vector3f xAxis = m_Up.cross(zAxis).normalized();
    Eigen::Vector3f yAxis = zAxis.cross(xAxis);

    // Construimos la matriz a mano (estilo lookAt)
    m_View.setIdentity();
    m_View(0, 0) = xAxis.x(); m_View(0, 1) = xAxis.y(); m_View(0, 2) = xAxis.z();
    m_View(1, 0) = yAxis.x(); m_View(1, 1) = yAxis.y(); m_View(1, 2) = yAxis.z();
    m_View(2, 0) = zAxis.x(); m_View(2, 1) = zAxis.y(); m_View(2, 2) = zAxis.z();

    m_View(0, 3) = -xAxis.dot(m_Position);
    m_View(1, 3) = -yAxis.dot(m_Position);
    m_View(2, 3) = -zAxis.dot(m_Position);
}

void Camera::UpdateProjectionMatrix()
{
    m_Projection.setIdentity();

    float radFov = m_FOV * static_cast<float>(M_PI) / 180.0f;
    float tanHalfFov = std::tan(radFov / 2.0f);

    m_Projection(0, 0) = 1.0f / (m_AspectRatio * tanHalfFov);
    m_Projection(1, 1) = 1.0f / tanHalfFov;
    m_Projection(2, 2) = -(m_FarPlane + m_NearPlane) / (m_FarPlane - m_NearPlane);
    m_Projection(2, 3) = -(2.0f * m_FarPlane * m_NearPlane) / (m_FarPlane - m_NearPlane);
    m_Projection(3, 2) = -1.0f;
    m_Projection(3, 3) = 0.0f;
}
