#ifndef CAMERA_H
#define CAMERA_H
#include <glad/glad.h>

#include "GLADhelper.h"
#include "../support/math_helper.h"

class Transform
{
public:
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW

	inline Transform(	const Vector3f& pos = Vector3f::Zero(),
						const Quaternionf rot = Quaternionf()):
						position(pos), rotation(rot) {};

	Transform lerp(		float alpha, const Transform& other)
	{
		return Transform(	(1.f - alpha) * position + alpha * other.position,
							rotation.slerp(alpha, other.rotation));
	}

	Vector3f position;
	Quaternionf rotation;
};

class Camera
{
public:
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW

	// Constructors
	Camera(float fov, float aspect, float zNear, float zFar);
	Camera(const Camera& other);
	Camera();

	Camera& operator=(const Camera& other);

	~Camera();

	void SetViewPort(uint offsetX, uint offsetY, uint width, uint height);
	void SetViewPort(uint width, uint height);

	// Getters & setters
	inline uint VPx(void) const { return _VPx; }
	inline uint VPy(void) const { return _VPy; }
	inline uint VPwidth(void) const { return _VPwidth; }
	inline uint VPheight(void) const { return _VPheight; }

	inline float FovY(void) const { return _FovY; }
	void SetFovY(float value);

	void SetPosition(const Vector3f& pos);
	inline const Vector3f& Position(void) const { return _transform.position; };

	void SetRotation(const Quaternionf& rot);
	inline const Quaternionf& Rotation(void) const { return _transform.rotation; };

	void SetTransform(const Transform& t);
	inline const Transform& GetTransform(void) const { return _transform; };

	void SetDirection(const Vector3f& dir);
	Vector3f Direction(void) const;

	void SetUp(const Vector3f& vectorUp);
	Vector3f Up(void) const;
	Vector3f Right(void) const;

	void SetTarget(const Eigen::Vector3f& targ);
	inline const Eigen::Vector3f& Target(void) { return _Target; }

	const Affine3f& ViewMatrix(void) const;
	const Matrix4f& ProjectionMatrix(void) const;

	void RotateAroundTarget(const Quaternionf& rotation);
	void RotateLocal(const Quaternionf& rotation);
	void Zoom(float d);

	void TranslateLocal(const Vector3f& t);
	
	void ActivateGL(void);

	Vector3f Unproject(Vector2f uv, float depth) const ;
	Vector3f Unproject(Vector2f uv, float depth, const Matrix4f inModelView) const;

private:
	// Viewport offset
	uint _VPx, _VPy;
	// Viewport size
	uint _VPwidth, _VPheight;
	// Object attributes
	Transform _transform;
	
	mutable Affine3f _ViewMatrix;
	mutable Matrix4f _ProjectionMatrix;
	
	// Matrix' dirty flag pattern
	mutable bool _ViewIsUpToDate;
	mutable bool _ProjIsUpToDate;

	// for RotateAroundTarget();
	Vector3f _Target;

	// Camera properties
	float _FovY;
	float _Near, _Far;

	void UpdateViewMatrix(void) const;
	void UpdateProjectionMatrix(void) const;
};

#endif // !CAMERA_H
