// Camera.h
#pragma once

#include <Eigen/Core>
#include <Eigen/Geometry>

class Camera
{
public:
    Camera();
    ~Camera() = default;

    // --- Getters/Setters b�sicos ---
    inline Eigen::Vector3f GetPosition() const { return m_Position; }
    void SetPosition(const Eigen::Vector3f& pos) { m_Position = pos; UpdateViewMatrix(); }

    inline Eigen::Vector2f GetRotation() const { return m_Rotation; }
    void SetRotation(const Eigen::Vector2f& rot) { m_Rotation = rot; UpdateViewMatrix(); }

    inline Eigen::Vector3f GetTarget() const { return m_Target; }
    void SetTarget(const Eigen::Vector3f& target) { m_Target = target; }

    // --- Manejo de proyecci�n ---
    inline float GetFOV() const { return m_FOV; }
    void  SetFOV(float fovDegrees) { m_FOV = fovDegrees; UpdateProjectionMatrix(); }

    inline float GetAspectRatio() const { return m_AspectRatio; }
    void  SetAspectRatio(float aspect) { m_AspectRatio = aspect; UpdateProjectionMatrix(); }

    void  SetClippingPlanes(float nearPlane, float farPlane)
    {
        m_NearPlane = nearPlane;
        m_FarPlane = farPlane;
        UpdateProjectionMatrix();
    }
    void Translate(const Eigen::Vector3f& translation)
    {
        m_Position += translation;
        UpdateViewMatrix();
    }

    // --- Obtener las matrices resultantes ---
    inline const Eigen::Matrix4f& GetViewMatrix() const { return m_View; }
    inline const Eigen::Matrix4f& GetProjectionMatrix() const { return m_Projection; }


private:
    void UpdateViewMatrix();
    void UpdateProjectionMatrix();

private:
    // Par�metros de la c�mara
    Eigen::Vector3f m_Position;
    Eigen::Vector2f m_Rotation;
    Eigen::Vector3f m_Up;
    Eigen::Vector3f m_Target;

    // Par�metros de proyecci�n
    float m_FOV;         // En grados
    float m_AspectRatio;
    float m_NearPlane;
    float m_FarPlane;

    // Matrices resultantes
    Eigen::Matrix4f m_View;
    Eigen::Matrix4f m_Projection;
};