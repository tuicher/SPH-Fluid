#include "Camera.h"

Camera::Camera()
	:_ViewIsUpToDate(false), _ProjIsUpToDate(false)
{
	_ViewMatrix.setIdentity();
    _FovY = M_PI / 3.;
    _Near = 1.;
    _Far = 50000.;

    _VPx = 0;
    _VPy = 0;

    SetPosition(Vector3f::Constant(100.));
    SetTarget(Vector3f::Zero());
}

Camera::Camera(const Camera& other)
{
    *this = other;
}

Camera& Camera::operator=(const Camera& other)
{
    _ViewIsUpToDate = false;
    _ProjIsUpToDate = false;

    _VPx = other._VPx;
    _VPy = other._VPy;
    _VPwidth = other._VPwidth;
    _VPheight = other._VPheight;

    _Target = other._Target;
    _FovY = other._FovY;
    _Near = other._Near;
    _Far = other._Far;

    _ViewMatrix = other._ViewMatrix;
    _ProjectionMatrix = other._ProjectionMatrix;

    return *this;
}

Camera::~Camera()
{
    return;
}

void Camera::SetViewPort(uint offsetX, uint offsetY, uint width, uint height)
{
    _VPx = offsetX;
    _VPy = offsetY;
    _VPwidth = width;
    _VPheight = height;

    _ProjIsUpToDate = false;
}

void Camera::SetViewPort(uint width, uint height)
{
    _VPwidth = width;
    _VPheight = height;

    _ProjIsUpToDate = false;
}

void Camera::SetFovY(float value)
{
    _FovY = value;

    _ProjIsUpToDate = false;
}

// rename forward (?)
Vector3f Camera::Direction(void) const
{
    return -(Rotation() * Vector3f::UnitZ());
}

Vector3f Camera::Up(void) const
{
    return Rotation() * Vector3f::UnitY();
}

Vector3f Camera::Right(void) const
{
    return Rotation() * Vector3f::UnitX();
}

void Camera::SetDirection(const Vector3f& dir)
{
    Vector3f up = this->Up();

    Matrix3f camAxes;

    camAxes.col(2) = (-dir).normalized();
    camAxes.col(0) = up.cross(camAxes.col(2)).normalized();
    camAxes.col(1) = camAxes.col(2).cross(camAxes.col(0)).normalized();
    SetRotation(Quaternionf(camAxes));

    _ViewIsUpToDate = false;
}

void Camera::SetTarget(const Vector3f& target)
{
    _Target = target;
    if (!_Target.isApprox(Position()))
    {
        Vector3f newDir = _Target - Position();
        SetDirection(newDir.normalized());
    }
}

void Camera::SetPosition(const Vector3f& p)
{
    _transform.position = p;

    _ViewIsUpToDate = false;
}

void Camera::SetRotation(const Quaternionf& rot)
{
    _transform.rotation = rot;

    _ViewIsUpToDate = false;
}

void Camera::SetTransform(const Transform& t)
{
    _transform = t;

    _ViewIsUpToDate = false;
}

void Camera::RotateAroundTarget(const Quaternionf& rotation)
{
    Matrix4f mRot, mT, mTm;

    // update the transform matrix
    UpdateViewMatrix();
    Vector3f t = _ViewMatrix * _Target;

    _ViewMatrix = Translation3f(t) * rotation * Translation3f(-t) * _ViewMatrix;

    Quaternionf qa(_ViewMatrix.linear());
    qa = qa.conjugate();
    SetRotation(qa);
    SetPosition(-(qa * _ViewMatrix.translation()));

    _ViewIsUpToDate = true;
}

void Camera::RotateLocal(const Quaternionf& rotation)
{
    float dist = (Position() - _Target).norm();
    SetRotation(Rotation() * rotation);
    _Target = Position() + dist * Direction();

    _ViewIsUpToDate = false;
}

void Camera::Zoom(float d)
{
    float dist = (Position() - _Target).norm();
    if (dist > d)
    {
        SetPosition(Position() + Direction() * d);
        
        _ViewIsUpToDate = false;
    }
}

void Camera::TranslateLocal(const Vector3f& t)
{
    Vector3f trans = Rotation() * t;
    SetPosition(Position() + trans);
    SetTarget(_Target + trans);

    _ViewIsUpToDate = false;
}

void Camera::UpdateViewMatrix(void) const
{
    if (!_ViewIsUpToDate)
    {
        Quaternionf q = Rotation().conjugate();
        _ViewMatrix.linear() = q.toRotationMatrix();
        _ViewMatrix.translation() = -(_ViewMatrix.linear() * Position());

        _ViewIsUpToDate = true;
    }
}

const Affine3f& Camera::ViewMatrix(void) const
{
    UpdateViewMatrix();
    return _ViewMatrix;
}

void Camera::UpdateProjectionMatrix(void) const
{
    if (!_ProjIsUpToDate)
    {
        _ProjectionMatrix.setIdentity();
        float aspect = float(_VPwidth) / float(_VPheight);
        float theta = _FovY * 0.5;
        float range = _Far - _Near;
        float invtan = 1. / tan(theta);

        _ProjectionMatrix(1, 1) = invtan;
        _ProjectionMatrix(2, 2) = -(_Near + _Far) / range;
        _ProjectionMatrix(3, 2) = -1;
        _ProjectionMatrix(2, 3) = -2 * _Near * _Far / range;
        _ProjectionMatrix(3, 3) = 0;

        _ProjIsUpToDate = true;
    }
}

const Matrix4f& Camera::ProjectionMatrix(void) const
{
    UpdateProjectionMatrix();
    return _ProjectionMatrix;
}

void Camera::ActivateGL(void)
{
    glViewport(VPx(), VPy(), VPwidth(), VPheight());
    
}