// Camera.cpp
#include "Camera.h"

#include "../support/Common.h"



Camera::Camera()
    : m_Position(0.0f, 0.0f, 2.5f)
    , m_Rotation(15.0f, 0.0f)
    , m_Up(0.0f, 1.0f, 0.0f)
    , m_Target(0.0f, 0.0f, 0.0f)
    , m_FOV(45.0f)
    , m_AspectRatio(1.3333f)
    , m_NearPlane(0.1f)
    , m_FarPlane(20.0f)
{
    UpdateViewMatrix();
    UpdateProjectionMatrix();
}

inline Eigen::Matrix4f LookAt(const Eigen::Vector3f& eye,
    const Eigen::Vector3f& center,
    const Eigen::Vector3f& up)
{
    Eigen::Matrix4f result;

    // Usamos la aproximaci�n "lookAt" con Eigen
    // Vector direcci�n
    Eigen::Vector3f zAxis = (eye - center).normalized();
    Eigen::Vector3f xAxis = up.cross(zAxis).normalized();
    Eigen::Vector3f yAxis = zAxis.cross(xAxis);

    // Construimos la matriz a mano (estilo lookAt)
    result.setIdentity();
    result(0, 0) = xAxis.x(); result(0, 1) = xAxis.y(); result(0, 2) = xAxis.z();
    result(1, 0) = yAxis.x(); result(1, 1) = yAxis.y(); result(1, 2) = yAxis.z();
    result(2, 0) = zAxis.x(); result(2, 1) = zAxis.y(); result(2, 2) = zAxis.z();

    result(0, 3) = -xAxis.dot(eye);
    result(1, 3) = -yAxis.dot(eye);
    result(2, 3) = -zAxis.dot(eye);

    return result;
}

void Camera::UpdateViewMatrix()
{
    // 1. Convertir rotaciones a radianes si están en grados
    float xAngleRad = m_Rotation.x() * DEG2RAD;  // asumiendo que m_Rotation.x() está en grados
    float yAngleRad = m_Rotation.y() * DEG2RAD;  // asumiendo que m_Rotation.y() está en grados

    // 2. Construir las rotaciones inversas (típico en cámara)
    //    Nota: si tu rotación “física” era glRotatef(xRot,1,0,0), en la matriz de vista
    //    se suele usar la rotación con signo invertido.
    Eigen::Matrix3f rotX = Eigen::AngleAxisf(xAngleRad, Eigen::Vector3f::UnitX()).toRotationMatrix();
    Eigen::Matrix3f rotY = Eigen::AngleAxisf(yAngleRad, Eigen::Vector3f::UnitY()).toRotationMatrix();

    // Multiplicamos rotY * rotX -> primero se aplica X, luego Y. Ajusta según tu orden deseado.
    //Eigen::Matrix3f rotTotal = rotY * rotX;
    Eigen::Matrix3f rotTotal = rotX * rotY;

    // 3. Construir la traslación inversa
    //    T(-m_Position)
    Eigen::Affine3f affine = Eigen::Affine3f::Identity();
    // “Acumulamos” la traslación en la transformación
    affine.translate(-m_Position);

    // 4. Combinar en una 4x4
    //    Recuerda que .matrix() te da la 4x4, pero rotTotal es 3x3 -> se puede
    //    hacer una 4x4 unidad e inyectar la parte rotacional.
    Eigen::Matrix4f matRot = Eigen::Matrix4f::Identity();
    matRot.block<3, 3>(0, 0) = rotTotal;

    Eigen::Matrix4f matTrans = affine.matrix();

    // La matriz de vista final = rot * trans (inversa de T * R)
    m_View = matTrans * matRot;

    //m_View = matRot * matTrans;

    /*
    m_View = LookAt(
        Eigen::Vector3f(0, 0, -3),  // eye
        Eigen::Vector3f(0, 0, 0),   // center
        Eigen::Vector3f(0, 1, 0)    // up
    );
    */
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
