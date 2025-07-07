#include "Transform.h"
#include "Matrix4.h"

QuantumEngine::Transform::Transform(const Vector3& position, const Vector3& scale, const Vector3& axis, Float angleDeg)
	:m_position(position), m_scale(scale), m_axis(axis), m_angle(angleDeg)
{
	UpdateDirections();
	UpdateMatrix();
}

QuantumEngine::Transform::Transform()
	:Transform(Vector3(0.0f), Vector3(1.0f), Vector3(0.0f, 0.0f, 1.0f), 0)
{
}

void QuantumEngine::Transform::UpdateDirections()
{
	Matrix4 mat = Matrix4::Rotate(m_axis, m_angle);
	m_forward = mat * Vector3(0.0f, 0.0f, 1.0f);
	m_up = mat * Vector3(0.0f, 1.0f, 0.0f);
	m_right = mat * Vector3(1.0f, 0.0f, 0.0f);
}

void QuantumEngine::Transform::UpdateMatrix()
{
	Matrix4 ST{
		m_scale.x, 0.0f, 0.0f, m_position.x,
		0.0f, m_scale.y, 0.0f, m_position.y,
		0.0f, 0.0f, m_scale.z, m_position.z,
		0.0f, 0.0f, 0.0f, 1.0f,
	};

	m_matrix = ST * Matrix4::Rotate(m_axis, m_angle);
}
