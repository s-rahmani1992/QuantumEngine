#pragma once
#include "../../BasicTypes.h"
#include "../Matrix4.h"

namespace QuantumEngine {
	class Transform;

	class Camera {
	public:
		Camera(const ref<Transform>& transform);
		inline Matrix4 ProjectionMatrix() const { return m_projectionMatrix; }
		inline ref<Transform> GetTransform() { return m_transform; }
		Matrix4 ViewMatrix();
	private:
		ref<Transform> m_transform;
	protected:
		Matrix4 m_projectionMatrix;
	};
}