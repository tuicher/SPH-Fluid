// Camera.h
#pragma once

#include <Eigen/Core>
#include <Eigen/Geometry>

class Camera
{
public:
    Camera();
    ~Camera() = default;

    Eigen::Vector3f GetPosition() const;
    void SetPosition(const Eigen::Vector3f& position);
    void SetTarget(const Eigen::Vector3f& target);
    void SetFOV(float fovDegrees);
    void SetAspectRatio(float aspect);
    void SetClippingPlanes(float nearPlane, float farPlane);

    void Translate(Eigen::Vector3f translation);

    const Eigen::Matrix4f& GetViewMatrix() const;
    const Eigen::Matrix4f& GetProjectionMatrix() const;

private:
    void UpdateViewMatrix();
    void UpdateProjectionMatrix();

private:
    // Parámetros de la cámara
    Eigen::Vector3f m_Position;
    Eigen::Vector3f m_Target;
    Eigen::Vector3f m_Up;

    // Parámetros de proyección
    float m_FOV;         // En grados
    float m_AspectRatio;
    float m_NearPlane;
    float m_FarPlane;

    // Matrices resultantes
    Eigen::Matrix4f m_View;
    Eigen::Matrix4f m_Projection;
};